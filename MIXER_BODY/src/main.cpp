#include <Arduino.h>
#include <ArduinoJson.h>

// ------------------- PIN DEFINITIONS -------------------
// RS485
#define RS485_TX 16
#define RS485_RX 17
#define RS485_DE 4

// Relays (Active High/Low depends on wiring, assuming same logic as production)
// Production logic: 
// Music: LOW = Bluetooth, HIGH = Line-In
// Mic: LOW = Wired, HIGH = Wireless
#define PIN_RELAY_MUSIC 12
#define PIN_RELAY_MIC   14

// ------------------- STATE -------------------
bool relayMusicState = false; // 0 = Line-In (High), 1 = Bluetooth (Low) - based on logic in prod
bool relayMicState = false;   // 0 = Wired (Low), 1 = Wireless (High)

// ------------------- FUNCTIONS -------------------

void updateRelays() {
    // Music Relay Logic from Production:
    // if relayMusicState (Bluetooth requested) -> LOW
    // else (Line In) -> HIGH
    digitalWrite(PIN_RELAY_MUSIC, relayMusicState ? LOW : HIGH);

    // Mic Relay Logic from Production:
    // if relayMicState (Wireless requested) -> HIGH
    // else (Wired) -> LOW
    digitalWrite(PIN_RELAY_MIC, relayMicState ? HIGH : LOW);

    Serial.printf("[RELAY UPDATE] Music: %s (Pin %d=%s) | Mic: %s (Pin %d=%s)\n",
        relayMusicState ? "BLUETOOTH" : "LINE-IN", 
        PIN_RELAY_MUSIC, relayMusicState ? "LOW" : "HIGH",
        relayMicState ? "WIRELESS" : "WIRED",
        PIN_RELAY_MIC, relayMicState ? "HIGH" : "LOW");
}

void processRs485Packet(String& input) {
    Serial.print("[RS485 RAW]: ");
    Serial.println(input);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, input);

    if (error) {
        Serial.print("[JSON ERROR]: ");
        Serial.println(error.c_str());
        return;
    }

    // Update State if keys exist
    if (doc.containsKey("mr")) relayMusicState = doc["mr"] == 1;
    if (doc.containsKey("cr")) relayMicState = doc["cr"] == 1;
    
    // Log Volumes if present (No action taken as analog is disabled)
    if (doc.containsKey("mv")) {
        Serial.printf("[VOL] Music: %d (Ignored)\n", (int)doc["mv"]);
    }
    if (doc.containsKey("cv")) {
        Serial.printf("[VOL] Mic: %d (Ignored)\n", (int)doc["cv"]);
    }

    updateRelays();
}

void processUsbCommand(char c) {
    switch (c) {
        case 'm':
            relayMusicState = !relayMusicState;
            Serial.println("[MANUAL] Toggled Music Relay");
            updateRelays();
            break;
        case 'c':
            relayMicState = !relayMicState;
            Serial.println("[MANUAL] Toggled Mic Relay");
            updateRelays();
            break;
        case '?':
            Serial.println("[HELP] m=Toggle Music, c=Toggle Mic");
            updateRelays(); // Show current state
            break;
    }
}

// ------------------- SETUP -------------------
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- MIXER BODY TEST FIRMWARE ---");
    Serial.println("Commands: 'm' = Toggle Music Relay, 'c' = Toggle Mic Relay");

    // Init Relays
    pinMode(PIN_RELAY_MUSIC, OUTPUT);
    pinMode(PIN_RELAY_MIC, OUTPUT);
    
    // Set Initial State (Default: Line-In, Wired)
    relayMusicState = false; 
    relayMicState = false;
    updateRelays();

    // Init RS485
    Serial2.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
    pinMode(RS485_DE, OUTPUT);
    digitalWrite(RS485_DE, LOW); // Rx Mode
}

// ------------------- LOOP -------------------
void loop() {
    // 1. Check RS485 (From Screen)
    if (Serial2.available()) {
        String input = Serial2.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            processRs485Packet(input);
        }
    }

    // 2. Check USB Serial (Manual Debug)
    if (Serial.available()) {
        char c = Serial.read();
        // Ignore newlines/spaces
        if (c != '\n' && c != '\r' && c != ' ') {
            processUsbCommand(c);
        }
    }

    delay(10);
}
