#include <Arduino.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

// DHT11-sensori
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// OLED-näyttö
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// LED
#define LED_PIN 23

// Painike
#define BUTTON_PIN 19

// Buzzer
#define BUZZER_PIN 17

// FreeRTOS-komponentit
QueueHandle_t dataQueue;         // Jono sensoridatan siirtoon
SemaphoreHandle_t mutex;         // Mutex näytön ja hälytyksen suojaamiseen
TimerHandle_t statusTimer;       // Ajastin järjestelmän tilan tarkistamiseen
TimerHandle_t ledBlinkTimer;     // Ajastin LED:n vilkkumiseen normaalitilassa
TimerHandle_t alarmDebounceTimer; // Ajastin hälytyksen uudelleenkäynnistykseen

// Keskeytyksen tila
volatile bool buttonPressed = false;
volatile unsigned long lastInterruptTime = 0;

// Globaalit muuttujat sensoridatan tallennukseen
volatile float currentTemperature = NAN;
volatile float currentHumidity = NAN;

// Hälytyksen tila
bool alarmActive = false;
bool alarmDebounceActive = false; // Estää hälytyksen, kun ajastin on aktiivinen

// Edellisen sensoridatan vertailu
float lastTemperature = 0.0;
float lastHumidity = 0.0;

// Sensoridatan rakenne
struct SensorData {
  float temperature;
  float humidity;
};

// Keskeytyksen käsittely painikkeelle
void IRAM_ATTR handleButtonPress() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 50) { // Debounce
    buttonPressed = true;
    lastInterruptTime = interruptTime;
  }
}

void dhtReadTask(void *pvParameters) {
  for (;;) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Tarkista, onnistuiko sensorin luku
    if (!isnan(temp) && !isnan(hum)) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      currentTemperature = temp;
      currentHumidity = hum;
      xSemaphoreGive(mutex);
    } else {
      Serial.println("Sensor read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); // 2 sekunnin viive
  }
}


// Sensoridatan lukutaski
void readSensorTask(void *pvParameters) {
  for (;;) {
    float temp, hum;

    // Noutaa sensoridatan globaaleista muuttujista
    xSemaphoreTake(mutex, portMAX_DELAY);
    temp = currentTemperature;
    hum = currentHumidity;
    xSemaphoreGive(mutex);

    // Varmista, että data on kelvollista
    if (!isnan(temp) && !isnan(hum)) {
      SensorData data;
      data.temperature = temp;
      data.humidity = hum;

      // Tarkista hälytysrajat
      if (temp < 24.9 || hum > 27.0) {
        if (!alarmDebounceActive) {
          xSemaphoreTake(mutex, portMAX_DELAY);
          alarmActive = true;
          xSemaphoreGive(mutex);
        }
      }

      // Lähetä data jonoon
      if (xQueueSend(dataQueue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
        Serial.println("Failed to send data to queue");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // Tarkistus 500 ms välein
  }
}

// Näytön päivitystaski
void updateDisplayTask(void *pvParameters) {
    for (;;) {
        SensorData data;

        // Lue data jonosta
        if (xQueueReceive(dataQueue, &data, portMAX_DELAY)) {
            // Tarkista, onko näyttö päivitettävä
            if (abs(data.temperature - lastTemperature) > 0.3 || 
                abs(data.humidity - lastHumidity) > 2 || 
                alarmActive) {

                lastTemperature = data.temperature;
                lastHumidity = data.humidity;

                xSemaphoreTake(mutex, portMAX_DELAY);

                // Päivitä näyttö
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.print("Temp: ");
                display.print(data.temperature);
                display.println(" C");
                display.print("Hum: ");
                display.print(data.humidity);
                display.println(" %");

                // Näytön viestit
                if (alarmActive) {
                    display.println("ALARM: ACTIVE!");
                } else if (data.temperature < 24.9 || data.humidity > 27.0) {
                    display.println("System Issue: Check Values");
                } else {
                    display.println("System OK");
                }

                display.display();
                xSemaphoreGive(mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Päivitys 500 ms välein
    }
}


// LED:n hallinta-ajastin
void ledBlinkCallback(TimerHandle_t xTimer) {
  static bool ledState = false;

  if (!alarmActive) {
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    ledState = !ledState;
  }
}

// Hälytyksen debounce-ajastimen käsittelijä
void alarmDebounceCallback(TimerHandle_t xTimer) {
  alarmDebounceActive = false; // Salli hälytys uudelleen
}

// Sarjamonitorin tilan tarkistus
void statusCallback(TimerHandle_t xTimer) {
  Serial.print("Temp: ");
  Serial.print(lastTemperature);
  Serial.print(" C, Hum: ");
  Serial.print(lastHumidity);
  Serial.println(" %");

  if (alarmActive) {
    Serial.println("ALARM: ACTIVE!");
  } else {
    Serial.println("System OK");
  }
}

// Hälytyksen hallintataski
void alarmTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT); // Määritä buzzerin pinni

  for (;;) {
    if (alarmActive) {
      digitalWrite(LED_PIN, HIGH); // Pysyvä valo hälytyksen aikana
      tone(BUZZER_PIN, 1000);      // Soita ääntä taajuudella 1000 Hz
    } else {
      // LED-ohjaus siirretty ajastimeen, joten ei tehdä mitään täällä
    }

    // Kuittaa hälytys painikkeella
    if (buttonPressed) {
      Serial.println("Button pressed, alarm cleared."); // Tulosta tieto painikkeen painalluksesta
      alarmActive = false;
      buttonPressed = false;
      noTone(BUZZER_PIN);          // Sammuta buzzer
      alarmDebounceActive = true; // Aktivoi hälytyksen estotila
      xTimerStart(alarmDebounceTimer, 0); // Käynnistä ajastin
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Tarkistus 100 ms välein
  }
}

void setup() {
  Serial.begin(9600);

  // DHT-sensorin alustus
  dht.begin();

  // OLED-näytön alustus
  Wire.begin(21, 22); // Määritä SDA ja SCL
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("System Starting...");
  display.display();

  // LED ja painike
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  // FreeRTOS-komponenttien alustus
  dataQueue = xQueueCreate(10, sizeof(SensorData));
  mutex = xSemaphoreCreateMutex();
  statusTimer = xTimerCreate("StatusTimer", pdMS_TO_TICKS(10000), pdTRUE, NULL, statusCallback);
  ledBlinkTimer = xTimerCreate("LEDBlinkTimer", pdMS_TO_TICKS(500), pdTRUE, NULL, ledBlinkCallback);
  alarmDebounceTimer = xTimerCreate("AlarmDebounce", pdMS_TO_TICKS(60000), pdFALSE, NULL, alarmDebounceCallback); // 60 sekunnin ajastin

  // Luo taskeja
  xTaskCreate(dhtReadTask, "DHTReadTask", 2048, NULL, 2, NULL); // prioriteetti nostettu 2
  xTaskCreate(readSensorTask, "SensorTask", 2048, NULL, 1, NULL); //muistia kasvatettu 2048
  xTaskCreate(updateDisplayTask, "DisplayTask", 2048, NULL, 1, NULL);
  xTaskCreate(alarmTask, "AlarmTask", 1000, NULL, 2, NULL); // prioriteetti nostettu 2

  // Käynnistä ajastimet
  xTimerStart(statusTimer, 0);
  xTimerStart(ledBlinkTimer, 0);
}

void loop() {
  // Ei tarvita mitään loopissa, FreeRTOS hallitsee ohjelmaa
}
