#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "BluetoothA2DPSink.h"

// ------------------- PIN DEFINITIONS (V2) -------------------
// RS485
#define RS485_TX 16
#define RS485_RX 17
#define RS485_DE 4

// Relays
#define PIN_RELAY_MUSIC 33 // V2 Pin
#define PIN_RELAY_MIC   14 // V2 Pin

// I2C (Volume)
#define I2C_SDA 21
#define I2C_SCL 22
#define PT2258_ADDR 0x40 // Detected Address

// I2S (For Bluetooth DAC - PCM5102)
#define I2S_BCK  26
#define I2S_LRCK 27
#define I2S_DOUT 25

// ------------------- OBJECTS -------------------
BluetoothA2DPSink a2dp_sink;

// ------------------- STATE -------------------
int currentMusicVol = 0; // 0-100 from Controller
int currentMicVol = 0;   // 0-100 from Controller
bool relayMusicState = false; // logic from Controller
bool relayMicState = false;   // logic from Controller

// Bluetooth State
bool isBluetoothActive = false;

// ------------------- PT2258 DRIVER -------------------
void pt2258_write(byte data) {
    Wire.beginTransmission(PT2258_ADDR);
    Wire.write(data);
    byte error = Wire.endTransmission();
    if (error) {
        // Serial.printf("[I2C ERROR] %d\n", error); // Optional: checking error
    }
}

// Helper to set volume for specific channel pair
// 0-100 input -> 79-0 dB attenuation
// PT2258: -10dB step = High Nibble (X), -1dB step = Low Nibble (Y)
// Code structure from datasheet: 
//   1-Channel: 10dB (1<ch><ch>1 <att>), 1dB (1<ch><ch>0 <att>) - THIS VARIES BY DATASHEET VERSION
//   Let's use the Master Volume for now if easiest, OR specific channels as verified.
//   Standard PT2258 Channel addressing (based on commonlibs):
//   Ch1 (Vol1): 0x80 / 0x90
//   Ch2 (Vol2): 0x40 / 0x50
//   Ch3 (Vol3): 0x00 / 0x10
//   Ch4 (Vol4): 0x20 / 0x30
//   Ch5 (Vol5): 0x60 / 0x70
//   Ch6 (Vol6): 0xA0 / 0xB0
//   Master:     0xD0 / 0xE0
//
//   Mapping from "Pins.txt":
//   Ch1, Ch2 -> Music
//   Ch3, Ch4 -> Mic

void setChannelVolume(int ch_10db_base, int ch_1db_base, int volume0to100) {
    // Map 0-100 to 79-0 attenuation
    // 100 -> 0 dB (Loudest)
    // 0   -> 79 dB (Quiet)
    int attenuation = map(volume0to100, 0, 100, 79, 0);
    if (attenuation < 0) attenuation = 0;
    if (attenuation > 79) attenuation = 79;
    
    int tens = attenuation / 10;
    int ones = attenuation % 10;

    pt2258_write(ch_10db_base | tens);
    pt2258_write(ch_1db_base  | ones);
}

void updateVolume() {
    // Music (Channels 1 & 2)
    // Ch1 (L): 0x80/0x90
    setChannelVolume(0x80, 0x90, currentMusicVol);
    // Ch2 (R): 0x40/0x50
    setChannelVolume(0x40, 0x50, currentMusicVol);

    // Mic (Channels 3 & 4)
    // Ch3 (L): 0x00/0x10
    setChannelVolume(0x00, 0x10, currentMicVol);
    // Ch4 (R): 0x20/0x30
    setChannelVolume(0x20, 0x30, currentMicVol);
}

// ------------------- LOGIC -------------------

void updateRelays() {
    // Logic:
    // Music Relay (33): LOW = Bluetooth (NC), HIGH = Line-In (NO).
    // relayMusicState == 1 (from Controller) => Bluetooth Mode.
    if (relayMusicState) {
        // Bluetooth
        digitalWrite(PIN_RELAY_MUSIC, LOW); 
        if (!isBluetoothActive) {
            Serial.println("Starting Bluetooth...");
            a2dp_sink.start("Mixer Audio");
            isBluetoothActive = true;
        }
    } else {
        // Line-In
        digitalWrite(PIN_RELAY_MUSIC, HIGH);
        if (isBluetoothActive) {
            Serial.println("Stopping Bluetooth...");
            a2dp_sink.end(); 
            isBluetoothActive = false;
        }
    }

    // Mic Relay (14)
    // relayMicState == 1 => Wireless (High), 0 => Wired (Low) [Assumption based on previous code]
    digitalWrite(PIN_RELAY_MIC, relayMicState ? HIGH : LOW);
}

void processPacket(String& input) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, input);

    if (error) {
        Serial.print("JSON Error: ");
        Serial.println(error.c_str());
        return;
    }
    
    if (doc.containsKey("mv")) currentMusicVol = doc["mv"];
    if (doc.containsKey("cv")) currentMicVol = doc["cv"];
    if (doc.containsKey("mr")) relayMusicState = doc["mr"] == 1;
    if (doc.containsKey("cr")) relayMicState = doc["cr"] == 1;

    Serial.printf("Upd: MusV=%d MicV=%d MusR=%d MicR=%d\n", 
                  currentMusicVol, currentMicVol, relayMusicState, relayMicState);

    updateRelays();
    updateVolume();
}

// ------------------- SETUP -------------------
void setup() {
    Serial.begin(115200);
    Serial.println("Mixer Body V2 Booting...");

    // Relays
    pinMode(PIN_RELAY_MUSIC, OUTPUT);
    pinMode(PIN_RELAY_MIC, OUTPUT);
    digitalWrite(PIN_RELAY_MUSIC, HIGH); // Default to Line-In (HIGH)
    digitalWrite(PIN_RELAY_MIC, LOW);

    // RS485
    Serial2.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
    pinMode(RS485_DE, OUTPUT);
    digitalWrite(RS485_DE, LOW); 

    // I2C Volume
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    pt2258_write(0xC0); // Clear / Reset
    delay(200);
    updateVolume(); // Apply initial 0-0 volume

    // Bluetooth Config
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = I2S_BCK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.set_auto_reconnect(false); 
    
    Serial.println("Ready.");
}

void handleSerialDebug() {
    if (Serial.available()) {
        char c = Serial.read();
        bool changed = false;

        switch (c) {
            case 'b': // Bluetooth On
            case 'B':
                relayMusicState = true;
                changed = true;
                Serial.println("cmd: Bluetooth ON");
                break;
            case 's': // Stop (Bluetooth Off / Line In)
            case 'S':
                relayMusicState = false;
                changed = true;
                Serial.println("cmd: Bluetooth OFF");
                break;
            case 'm': // Mic On
            case 'M':
                relayMicState = true;
                changed = true;
                Serial.println("cmd: Mic ON");
                break;
            case 'w': // Wired Mic (Mic Off)
            case 'W':
                relayMicState = false;
                changed = true;
                Serial.println("cmd: Mic OFF");
                break;
            case '+': // Volume Up (Music)
                currentMusicVol = min(100, currentMusicVol + 10);
                changed = true;
                Serial.printf("cmd: Music Vol %d\n", currentMusicVol);
                break;
            case '-': // Volume Down (Music)
                currentMusicVol = max(0, currentMusicVol - 10);
                changed = true;
                Serial.printf("cmd: Music Vol %d\n", currentMusicVol);
                break;
        }

        if (changed) {
            updateRelays();
            updateVolume();
        }
    }
}

// ------------------- LOOP -------------------
void loop() {
    // RS485 Listener
    if (Serial2.available()) {
        // Debug: Read raw character to see if anything arrives
        char c = Serial2.peek();
        // Serial.printf("[DEBUG RS485 RAW] 0x%02X (%c)\n", c, c); // Optional: very verbose

        String input = Serial2.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            Serial.print("[RS485 RX]: ");
            Serial.println(input);
            processPacket(input);
        }
    }
    
    // USB Serial Debug Listener
    handleSerialDebug();

    delay(10);
}
