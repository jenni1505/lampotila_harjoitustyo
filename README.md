# FreeRTOS-pohjainen DHT11-sensoriprojekti

Tämä on harjoitustyö, jossa käytetään FreeRTOS-käyttöjärjestelmää ja DHT11-sensoria lämpötilan ja kosteuden mittaamiseen. Projekti hyödyntää FreeRTOS:n keskeisiä ominaisuuksia, kuten taskeja, jonoja, ajastimia, keskeytyksiä ja mutex-lukkoja. Tavoitteena on luoda vakaa ja skaalautuva järjestelmä, jossa eri toiminnallisuudet toimivat rinnakkain ilman häiriöitä.

---

## Projektin ominaisuudet

- **DHT11-sensorin lukeminen:** 
  - Sensorilta luetaan lämpötila- ja kosteusarvot ja niitä prosessoidaan reaaliaikaisesti.
  
- **FreeRTOS-komponentit:**
  - **Tehtävät (Tasks):** Eri toiminnoille, kuten sensorin lukemiseen, näytön päivittämiseen, hälytysten hallintaan ja datan prosessointiin.
  - **Jonot (Queues):** Sensoridatan siirtoon tehtävien välillä.
  - **Ajastimet (Timers):** LED:n vilkkumiseen ja järjestelmän tilan tarkistamiseen.
  - **Keskeytykset (Interrupts):** Painikkeen painalluksen käsittely hälytyksen kuittaamiseen.
  - **Mutex-lukot:** Suojaamaan yhteisten resurssien käyttöä, kuten näyttöä ja sensorin dataa.

- **OLED-näyttö:** 
  - Näyttää reaaliaikaisesti lämpötila- ja kosteusarvot sekä järjestelmän tilan.
  - Hälytyksen ja ongelmatilanteiden visuaalinen ilmoitus.

- **Hälytysjärjestelmä:**
  - Hälytys aktivoituu, jos mittaustulokset ylittävät määritetyt raja-arvot.
  - Käyttäjä voi kuitata hälytyksen painikkeella.

---

## Käytetyt laitteet ja kirjastot

### **Laitteet:**
- **DHT11-sensori**: Lämpötilan ja kosteuden mittaamiseen.
- **NodeMCU**: Mikrokontrollerina.
- **OLED-näyttö (SSD1306)**: Mittaustulosten ja järjestelmän tilan näyttämiseen.
- **Painike ja LED**: Käyttäjän interaktiota varten.
- **Buzzer**: Äänihälytyksiä varten.

### **Kirjastot:**
- `DHT` - DHT11-sensorin lukemiseen.
- `Adafruit_GFX` ja `Adafruit_SSD1306` - OLED-näytön hallintaan.
- `FreeRTOS` - Käyttöjärjestelmänä.

---

## Järjestelmän toiminta

1. **Sensoridatan lukeminen:**
   - Lämpötila ja kosteus luetaan erillisessä tehtävässä ja tallennetaan globaaleihin muuttujiin mutex-suojauksen avulla.

2. **Sensoridatan prosessointi:**
   - Sensoridata siirretään jonoihin muiden tehtävien käyttöön, ja tarkistetaan, ylittävätkö arvot määritetyt hälytysrajat.

3. **Näytön päivitys:**
   - OLED-näytölle päivitetään lämpötila, kosteus ja järjestelmän tila vain, jos data tai järjestelmän tila muuttuu.

4. **Hälytysten hallinta:**
   - Hälytys aktivoituu, jos raja-arvot ylitetään.
   - Painikkeen painaminen kuittaa hälytyksen, mutta ongelmatilanteesta ilmoitetaan edelleen.

---

## Asennus ja käyttö

### 1. **Laitteiston kytkeminen:**
- Kytke DHT11-sensori, OLED-näyttö, painike, LED ja buzzer mikrokontrolleriin seuraavien pinni-asetusten mukaisesti:
  - **DHT11:** Pin 18
  - **OLED (I2C):** SDA 21, SCL 22
  - **Painike:** Pin 19
  - **LED:** Pin 23
  - **Buzzer:** Pin 17

### 2. **Ohjelmiston lataaminen:**
- Lataa tämä repository GitHubista:
  ```bash
  git clone https://github.com/kayttaja/freeRTOS-DHT11-project.git
  ![WhatsApp-Kuva 2024-11-25 klo 10 40 53_edfc680c](https://github.com/user-attachments/assets/b512f5a0-9d48-49e3-8d6a-8391a530a073)
  ![WhatsApp-Kuva 2024-11-25 klo 10 40 53_aeaf4c23](https://github.com/user-attachments/assets/077c3b40-8670-4a5c-92e7-0d01a0c2bc92)


