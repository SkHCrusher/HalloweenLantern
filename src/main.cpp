#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <esp_now.h>

// ============== KONFIGURATION ==============
#define LED_PIN         8         // GPIO8 am ESP32-C3 Super Mini
#define NUM_LEDS_X      10        // LEDs pro Reihe (Umfang)
#define NUM_LEDS_Y      12        // Anzahl Reihen (Höhe)
#define NUM_LEDS        (NUM_LEDS_X * NUM_LEDS_Y)  // 120 LEDs total

#define BRIGHTNESS      255       // Helligkeit 0-255
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

// Feuer-Parameter
#define COOLING         55        // Abkühlrate (höher = schneller auskühlen)
#define SPARKING        120       // Funken-Wahrscheinlichkeit (höher = mehr Funken)
#define FIRE_SPEED      50        // Millisekunden zwischen Updates

// Button-Konfiguration
#define BUTTON_PIN      20        // GPIO20 für Button (gegen GND)
#define DEBOUNCE_MS     30        // Pegel muss so lange stabil sein, bevor er zählt

// ESP-NOW Synchronisation
#define ESPNOW_ENABLED  true      // true = Sync aktiviert, false = nur lokal

// ============== GLOBALE VARIABLEN ==============
CRGB leds[NUM_LEDS];
byte heat[NUM_LEDS_X][NUM_LEDS_Y];
uint8_t currentColor = 0;
CRGBPalette16 firePalette = HeatColors_p;

// Button-Variablen (Stable-State-Debouncing)
bool buttonRaw = HIGH;            // letzter Roh-Pegel
bool buttonStable = HIGH;         // bestätigter, stabiler Pegel
unsigned long buttonChangeTime = 0;

// ============== ESP-NOW ==============
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define SYNC_MAGIC 0x484C4C4Eu  // "HLLN" = Halloween Lantern

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t color;
  uint8_t checksum;  // XOR über magic-Bytes + color
} SyncMessage;

static uint8_t syncChecksum(uint32_t magic, uint8_t color) {
  return (uint8_t)(magic) ^ (uint8_t)(magic >> 8) ^
         (uint8_t)(magic >> 16) ^ (uint8_t)(magic >> 24) ^ color;
}

SyncMessage syncData;
bool syncReceived = false;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SyncMessage)) return;
  SyncMessage received;
  memcpy(&received, data, sizeof(SyncMessage));
  if (received.magic != SYNC_MAGIC) return;
  if (received.checksum != syncChecksum(received.magic, received.color)) return;
  if (received.color > 7) return;
  syncData = received;
  syncReceived = true;
}

void initESPNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen!");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer hinzufügen fehlgeschlagen!");
    return;
  }

  Serial.println("ESP-NOW bereit!");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void broadcastMode() {
  if (!ESPNOW_ENABLED) return;

  SyncMessage msg;
  msg.magic = SYNC_MAGIC;
  msg.color = currentColor;
  msg.checksum = syncChecksum(msg.magic, msg.color);

  esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
}

// ============== XY MAPPING ==============
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= NUM_LEDS_X || y >= NUM_LEDS_Y) return 0;

  uint8_t physY = (NUM_LEDS_Y - 1) - y;

  if (physY & 1) {
    return (physY * NUM_LEDS_X) + (NUM_LEDS_X - 1 - x);
  } else {
    return (physY * NUM_LEDS_X) + x;
  }
}

// ============== FEUER SIMULATION ==============
void fireEffect() {
  for (uint8_t x = 0; x < NUM_LEDS_X; x++) {
    for (uint8_t y = 0; y < NUM_LEDS_Y; y++) {
      heat[x][y] = qsub8(heat[x][y], random8(0, ((COOLING * 10) / NUM_LEDS_Y) + 2));
    }

    for (uint8_t y = NUM_LEDS_Y - 1; y >= 2; y--) {
      uint8_t leftX = (x + NUM_LEDS_X - 1) % NUM_LEDS_X;
      uint8_t rightX = (x + 1) % NUM_LEDS_X;
      heat[x][y] = (heat[x][y - 1] + heat[x][y - 2] + heat[leftX][y - 1] + heat[rightX][y - 1]) / 4;
    }

    if (random8() < SPARKING) {
      uint8_t sparkY = random8(3);
      heat[x][sparkY] = qadd8(heat[x][sparkY], random8(160, 255));
    }
  }

  for (uint8_t x = 0; x < NUM_LEDS_X; x++) {
    for (uint8_t y = 0; y < NUM_LEDS_Y; y++) {
      byte colorIndex = scale8(heat[x][y], 240);
      leds[XY(x, y)] = ColorFromPalette(firePalette, colorIndex);
    }
  }
}

// ============== TWINKLE FIRE ==============
void twinkleFireEffect() {
  fireEffect();

  if (random8() < 80) {
    uint8_t sparkX = random8(NUM_LEDS_X);
    uint8_t sparkY = random8(NUM_LEDS_Y / 2);
    leds[XY(sparkX, sparkY)] = CRGB::White;
  }
}

// ============== FEUERFARBEN ==============
void setFireColor(uint8_t colorMode) {
  switch (colorMode) {
    case 0:  // Orange-Rot
      firePalette = HeatColors_p;
      break;
    case 1:  // Blau
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB::DarkBlue, CRGB::DarkBlue,
        CRGB::Blue, CRGB::Blue, CRGB::DodgerBlue, CRGB::DodgerBlue,
        CRGB::Cyan, CRGB::Cyan, CRGB::LightBlue, CRGB::LightBlue,
        CRGB::White, CRGB::White, CRGB::White, CRGB::White
      );
      break;
    case 2:  // Grün
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB::DarkGreen, CRGB::DarkGreen,
        CRGB::Green, CRGB::Green, CRGB::LimeGreen, CRGB::LimeGreen,
        CRGB::Yellow, CRGB::Yellow, CRGB::LightGreen, CRGB::LightGreen,
        CRGB::White, CRGB::White, CRGB::White, CRGB::White
      );
      break;
    case 3:  // Lila
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB::Indigo, CRGB::Indigo,
        CRGB::Purple, CRGB::Purple, CRGB::Magenta, CRGB::Magenta,
        CRGB::Pink, CRGB::Pink, CRGB::LightPink, CRGB::LightPink,
        CRGB::White, CRGB::White, CRGB::White, CRGB::White
      );
      break;
    case 4:  // Halloween
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB::DarkOrange, CRGB::DarkOrange,
        CRGB::Orange, CRGB::Orange, CRGB::OrangeRed, CRGB::OrangeRed,
        CRGB::Green, CRGB::Green, CRGB::LimeGreen, CRGB::LimeGreen,
        CRGB::White, CRGB::White, CRGB::White, CRGB::White
      );
      break;
    case 5:  // Eisfeuer
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB::Teal, CRGB::Teal,
        CRGB::Cyan, CRGB::Cyan, CRGB::Aqua, CRGB::Aqua,
        CRGB::LightCyan, CRGB::LightCyan, CRGB::LightBlue, CRGB::LightBlue,
        CRGB::White, CRGB::White, CRGB::White, CRGB::White
      );
      break;
    case 6:  // Tiefes Lila
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB(20, 0, 30), CRGB(20, 0, 30),
        CRGB(40, 0, 60), CRGB(40, 0, 60), CRGB(60, 0, 90), CRGB(60, 0, 90),
        CRGB(80, 0, 120), CRGB(80, 0, 120), CRGB(100, 20, 140), CRGB(100, 20, 140),
        CRGB(140, 50, 180), CRGB(140, 50, 180), CRGB(180, 100, 220), CRGB(180, 100, 220)
      );
      break;
    case 7:  // Tiefblau
      firePalette = CRGBPalette16(
        CRGB::Black, CRGB::Black, CRGB(0, 0, 30), CRGB(0, 0, 30),
        CRGB(0, 10, 60), CRGB(0, 10, 60), CRGB(0, 30, 100), CRGB(0, 30, 100),
        CRGB(0, 50, 140), CRGB(0, 50, 140), CRGB(20, 80, 180), CRGB(20, 80, 180),
        CRGB(60, 120, 220), CRGB(60, 120, 220), CRGB(150, 200, 255), CRGB(150, 200, 255)
      );
      break;
  }
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Halloween Lantern - Twinkle Fire");
  Serial.printf("Matrix: %dx%d = %d LEDs\n", NUM_LEDS_X, NUM_LEDS_Y, NUM_LEDS);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  memset(heat, 0, sizeof(heat));
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setFireColor(0);

  if (ESPNOW_ENABLED) {
    initESPNow();
  }

  Serial.println("\nButton (GPIO20): Farbe durchschalten");
  Serial.println("Farben: 0=Orange 1=Blau 2=Grün 3=Lila 4=Halloween 5=Eis 6=TiefLila 7=TiefBlau");
}

// ============== LOOP ==============
void loop() {
  // ESP-NOW Sync
  if (syncReceived) {
    syncReceived = false;
    if (syncData.color != currentColor) {
      currentColor = syncData.color;
      setFireColor(currentColor);
      Serial.printf("Sync: Farbe %d\n", currentColor);
    }
  }

  // Button-Steuerung mit Stable-State-Debouncing:
  // Ein neuer Pegel wird erst übernommen, wenn er DEBOUNCE_MS lang stabil anliegt.
  // Einzelne Spikes (EMI auf der Buttonleitung) werden so zuverlässig verworfen.
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != buttonRaw) {
    buttonRaw = reading;
    buttonChangeTime = millis();
  }
  if (buttonRaw != buttonStable && millis() - buttonChangeTime > DEBOUNCE_MS) {
    buttonStable = buttonRaw;
    if (buttonStable == LOW) {
      currentColor = (currentColor + 1) % 8;
      setFireColor(currentColor);
      Serial.printf("Farbe: %d\n", currentColor);
      broadcastMode();
    }
  }

  // Serial-Steuerung (Farben 0-7)
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c >= '0' && c <= '7') {
      currentColor = c - '0';
      setFireColor(currentColor);
      Serial.printf("Farbe: %d\n", currentColor);
      broadcastMode();
    }
  }

  twinkleFireEffect();
  FastLED.show();
  delay(FIRE_SPEED);
}
