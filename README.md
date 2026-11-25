# HMI touch 480×480 per Home Assistant (ESPHome + LVGL)

Firmware ESPHome per il pannello **Guition ESP32-S3-4848S040** (4", 480×480, touch), pensato come pannello di controllo da parete per Home Assistant. Tutto si configura dal display, senza riprogrammarlo.

## Funzioni

- **Home e pagina "Dispositivi"**: 24 slot (12 in home, 12 nella pagina Dispositivi). Ogni slot si configura dal display:
  - **On/Off**: interruttori, luci, ventole, `input_boolean` (un tocco fa il toggle);
  - **Sensore**: mostra il valore con la sua unità;
  - **Telecamera**: apre l'immagine di una ESP32-CAM (snapshot continui).

  Per configurare uno slot tocca uno slot vuoto (+) oppure tieni premuto uno slot già usato. Scegli entità e icona, poi con "Avanti" scrivi con la tastiera a schermo il nome breve da mostrare sul pulsante. La configurazione resta salvata sul dispositivo ed è visibile anche in HA (entità `text` "Slot N").
- **Calendario**: mese corrente, con un pallino colorato (il colore del calendario) per ogni evento. Toccando un giorno si vedono i dettagli: titolo, orario, calendario e luogo.
- **Schermata di blocco**: ora, temperatura e umidità interne (sensore AHT del pannello) ed esterne (da HA), più le previsioni meteo di 3 giorni.
- **Luminosità** giorno, sera e notte, con passaggio automatico ad alba e tramonto, e timeout di standby regolabile dal display.
- Pagina **Info** (Wi-Fi, IP, versione), schermata di avvio e barra di avanzamento durante l'aggiornamento OTA.

## Struttura

```
hmi.yaml                         configurazione del display (parti da qui)
secrets.yaml.example             da copiare in secrets.yaml
homeassistant/hmi_template.yaml  sensori template richiesti in Home Assistant
esphome-modular-lvgl-buttons/    moduli: pagine, pulsanti, hardware, logica C++
```

## Installazione

1. **Home Assistant**
   - Copia `homeassistant/hmi_template.yaml` in `/config` e aggiungi in `configuration.yaml` la riga `template: !include hmi_template.yaml`.
   - In quel file sostituisci i calendari d'esempio (`calendar.personale`, `calendar.lavoro`, `calendar.compleanni`) con i tuoi e scegli i loro colori.
   - Il meteo usa `weather.forecast_home`, l'integrazione meteo predefinita di HA.
   - Ricarica le entità template.
2. **ESPHome**
   - Copia `hmi.yaml` e la cartella `esphome-modular-lvgl-buttons/` nella cartella `esphome` di HA.
   - Copia `secrets.yaml.example` in `secrets.yaml` e compila i valori.
   - In `hmi.yaml` imposta le `substitutions`: `device_name`, e i sensori esterni `outdoor_temp_entity` e `outdoor_hum_entity`.
   - Se cambi `device_name`, aggiorna anche `hmi_display` nel template di HA, sostituendo i trattini con underscore.
3. Compila e carica il firmware. Poi in HA, nelle opzioni del dispositivo ESPHome, abilita **"Consenti al dispositivo di eseguire azioni di Home Assistant"**: serve per il toggle degli slot.

### Telecamere (opzionale)

Le telecamere selezionabili si definiscono con le `vars` di `pages/telecamere.yaml`: `cam1_name`, `cam1_url`, `cam2_name`, `cam2_url`. Gli URL sono quelli dello snapshot JPEG di una ESP32-CAM con `esp32_camera_web_server`.

## Requisiti

- ESPHome 2026.11 o successivo (LVGL 9).
- Pannello Guition ESP32-S3-4848S040 con 16 MB di flash e PSRAM octal.

## Crediti

Basato su [esphome-modular-lvgl-buttons](https://github.com/agillis/esphome-modular-lvgl-buttons) di Andrew Gillis (licenza MIT). Icone [Material Design Icons](https://pictogrammers.com/library/mdi/), font [Nunito](https://fonts.google.com/specimen/Nunito) (scaricato da Google Fonts durante la compilazione).

Licenza: MIT, vedi [LICENSE](LICENSE).
