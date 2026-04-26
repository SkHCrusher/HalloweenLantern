# Halloween Lantern

Animierter Feuer-/Twinkle-Effekt für eine Matrix aus 120 WS2812B-LEDs (10×12) auf einem ESP32-C3 Super Mini. Mehrere Laternen synchronisieren ihre Farbe untereinander via ESP-NOW; per Knopfdruck an einer Laterne wird die neue Farbe an alle anderen verteilt.

## Hardware

- Board: Lolin C3 Mini (ESP32-C3 Super Mini)
- LEDs: 120× WS2812B als 10×12-Matrix (10 LEDs pro Reihe = Umfang, 12 Reihen Höhe)
- Datenleitung: GPIO8
- Button: GPIO20 gegen GND, mit internem Pull-up

## Software

PlatformIO-Projekt, Arduino-Framework, FastLED.

```
pio run -t upload
pio device monitor
```

## Bedienung

- **Button** (GPIO20): schaltet die Farbe durch und broadcastet sie per ESP-NOW an alle anderen Laternen im Netz.
- **Serial** (USB CDC, 115200): Zeichen `0`–`7` setzt die Farbe direkt.

Verfügbare Farben: `0` Orange · `1` Blau · `2` Grün · `3` Lila · `4` Halloween · `5` Eis · `6` Tief-Lila · `7` Tief-Blau.

## ToDo / Geplante Verbesserungen

### Hardware-Revision (nächstes PCB)

Hintergrund: Im Betrieb wechselten alle drei Laternen nach längerer Laufzeit spontan die Farbe. Sehr wahrscheinliche Ursache: ein EMI-Spike auf der Buttonleitung einer Laterne löste einen Geister-Press aus, der per ESP-NOW-Broadcast an alle anderen verteilt wurde. Software-seitig ist das mit Stable-State-Debouncing und einem robusteren Sync-Paket entschärft. Für die nächste Hardware-Revision zusätzlich vormerken:

- [ ] **Button NICHT auf GPIO20** legen. GPIO20 ist `U0RXD` (UART0-RX) auf dem ESP32-C3 Super Mini und damit störanfälliger als ein normaler GPIO. Bessere Kandidaten: GPIO9 (hat Onboard-Pull-up und ist Boot-Button — darf beim Boot nicht aktiv LOW sein), oder GPIO3/4/5. GPIO0/2/8/9 sind Boot-Strap-Pins, davon nur einen verwenden und Boot-Verhalten beachten.
- [ ] **Externer Pull-up** am Button-Pin: 10 kΩ nach 3,3 V, zusätzlich zum internen Pull-up. Macht den Pin deutlich robuster gegen EMI-Einkopplungen — relevant, weil 120 WS2812B pro Laterne in unmittelbarer Nähe schalten.
- [ ] **Optional Tiefpass** am Button-Pin: 100 nF von Pin nach GND. Filtert kurze Spikes hardwareseitig.

### Software (offen)

- [ ] Persistenz der zuletzt gewählten Farbe (z. B. NVS), damit nach Reboot die letzte Farbe wiederhergestellt wird.
