#include <Arduino.h>
#include <ArduinoJson.h>
#include "BluetoothA2DPSink.h"
#include "M62429_Driver.h"

// ------------------- PIN DEFINITIONS -------------------
// RS485
#define RS485_TX 16
#define RS485_RX 17
#define RS485_DE 4

// Relays (Active High)
#define PIN_RELAY_MUSIC 12
#define PIN_RELAY_MIC   14

// Volume (M62429) - Shared Clock
#define M62429_CLK 22
#define M62429_DATA_MUSIC 21
#define M62429_DATA_MIC   19

// I2S (For Bluetooth DAC)
#define I2S_BCK  26
#define I2S_LRCK 27
#define I2S_DOUT 25

// ------------------- OBJECTS -------------------
BluetoothA2DPSink a2dp_sink;

// Volume Drivers (One for Music, One for Mic)
// They share the CLOCK pin.
M62429 volMusic(M62429_DATA_MUSIC, M62429_CLK);
M62429 volMic(M62429_DATA_MIC, M62429_CLK);

// ------------------- STATE -------------------
int currentMusicVol = 0;
int currentMicVol = 0;
bool relayMusicState = false;
bool relayMicState = false;

// Bluetooth State
bool isBluetoothActive = false;

// ------------------- FUNCTIONS -------------------

void updateRelays() {
    // Logic Correction based on User Feedback:
    // Music Relay: Bluetooth is connected to NC (Normally Closed).
    //              Line-In is connected to NO (Normally Open).
    //              Therefore: LOW = Bluetooth, HIGH = Line-In.
    
    // Mic Relay: Wired is NC, Wireless is NO (User can swap HW, but we assume this standard).
    //            Therefore: LOW = Wired, HIGH = Wireless.

    // Assumption: 'relayMusicState' (from Controller) == 1 means "Bluetooth Mode Requested".
    //             'relayMicState' (from Controller) == 1 means "Wireless Mic Requested".

    if (relayMusicState) {
        // Bluetooth Mode Requested (1)
        // Hardware: BT is NC -> Write LOW.
        digitalWrite(PIN_RELAY_MUSIC, LOW); 

        // Start A2DP if not active
        if (!isBluetoothActive) {
            Serial.println("Starting Bluetooth...");
            a2dp_sink.start("Mixer Audio");
            isBluetoothActive = true;
        }
    } else {
        // Line-In Mode Requested (0)
        // Hardware: Line is NO -> Write HIGH.
        digitalWrite(PIN_RELAY_MUSIC, HIGH);

        // Stop A2DP
        if (isBluetoothActive) {
            Serial.println("Stopping Bluetooth...");
            a2dp_sink.end(); 
            isBluetoothActive = false;
        }
    }

    // Mic Relay
    // 1 = Wireless (NO) -> HIGH
    // 0 = Wired (NC) -> LOW
    digitalWrite(PIN_RELAY_MIC, relayMicState ? HIGH : LOW);
}

void updateVolume() {
    volMusic.setVolume(currentMusicVol);
    volMic.setVolume(currentMicVol);
}

void processPacket(String& input) {
    // Parse JSON
    // Expected: {"mv":80, "cv":50, "mr":1, "cr":0}
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
    // Debug Serial
    Serial.begin(115200);
    Serial.println("Mixer Body Booting...");

    // Init Relays
    pinMode(PIN_RELAY_MUSIC, OUTPUT);
    pinMode(PIN_RELAY_MIC, OUTPUT);
    digitalWrite(PIN_RELAY_MUSIC, LOW);
    digitalWrite(PIN_RELAY_MIC, LOW);

    // Init RS485
    // Use Serial2 (RX=16, TX=17 is Standard for U2 on ESP32 sometimes, but we map explicitly)
    Serial2.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
    pinMode(RS485_DE, OUTPUT);
    digitalWrite(RS485_DE, LOW); // Listen mode default

    // Init Volume Drivers
    volMusic.begin();
    volMic.begin();

    // Init A2DP (Config only)
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = I2S_BCK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.set_auto_reconnect(false); // No PIN code flow / Simple Secure Pairing
    // a2dp_sink.set_volume(127); // Max Digital Volume, we control Analog.
    
    // Supposedly "No Tones" is default for this lib unless audio clips are provided?
    // Or we might need to silence connection events if they exist.
    // The lib doesn't play tones by default unless callbacks do it.

    Serial.println("Ready. Requesting state from controller...");
    
    // Send request for update to Controller (Screen)
    // IMPORTANT: Switch RS485 to TX mode briefly
    digitalWrite(RS485_DE, HIGH);
    Serial2.println("?"); // Controller listens for "?" or "get"
    Serial2.flush();     // Wait for transmission
    digitalWrite(RS485_DE, LOW); // Back to Listen
}

// ------------------- LOOP -------------------
void loop() {
    // Handle Bluetooth (Managed by Tasks usually, but `handle` might be needed if not using RTOS mode - Lib uses Tasks)
    // No loop handler needed for A2DP Sink usually.

    // Handle RS485
    if (Serial2.available()) {
        String input = Serial2.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            processPacket(input);
            
            // Send Ack? User requested "Two way".
            // Ideally we only Ack if we are asked, or we might flood.
            // But getting a state is rare.
            // Let's print local debug mainly.
            // If we want to send ACK to screen, user needs to implement receiving on Screen side properly.
            // Screen handles `?` to send. It does not seemingly handle ACKs.
            // We will stick to receiving for now unless protocol changes.
        }
    }

    delay(10);
}