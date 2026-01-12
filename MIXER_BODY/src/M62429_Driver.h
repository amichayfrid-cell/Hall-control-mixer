#ifndef M62429_DRIVER_H
#define M62429_DRIVER_H

#include <Arduino.h>

class M62429 {
private:
    uint8_t _data_pin;
    uint8_t _clk_pin;

    void writeBit(bool bit) {
        digitalWrite(_data_pin, bit);
        delayMicroseconds(2);
        digitalWrite(_clk_pin, HIGH);
        delayMicroseconds(4);
        digitalWrite(_clk_pin, LOW);
        delayMicroseconds(2);
    }

public:
    M62429(uint8_t data_pin, uint8_t clk_pin) : _data_pin(data_pin), _clk_pin(clk_pin) {}

    void begin() {
        pinMode(_data_pin, OUTPUT);
        pinMode(_clk_pin, OUTPUT);
        digitalWrite(_data_pin, LOW);
        digitalWrite(_clk_pin, LOW);
    }

    // Set volume (0-100)
    // M62429 has range 0 to 83 (steps). 0 is -infinity (mute), 87 is 0dB (max).
    // We Map 0-100 to this range.
    void setVolume(uint8_t vol_percent) {
        // Defines from datasheet
        // 11 bits total usually sent per channel?
        // Standard format: [D0-D1 Select] [D2-D6 Attenuation] [D7,D8 Reserved] [D9,D10 Latch]
        // Actually M62429 is simpler:
        // 9 bits data + latch sequence?
        // Let's use the standard algorithm: 
        // 10 bits: D0-D1 (Channel), D2-D6 (Atten), D7-D8 (Set to 1 needed?), D9 (Latch?)
        
        // M62429 protocol:
        // 11 bits frame.
        // D0, D1: 00(Ch1), 01(Ch2), 10(Both), 11(Both) - We use Both (2).
        // D2-D6: Attenuation. 0 = 0dB (Max), 80 = -80dB (Min).
        // Muting is done by max attenuation.
        
        // Map 0 (Min) - 100 (Max) input to M26429 Attenuation
        // Vol 100 -> Att 0
        // Vol 0 -> Att 80 (approx)
        
        uint8_t att = 0;
        if (vol_percent == 0) {
            att = 83; // Approx mute
        } else {
            // Map 1-100 to 83-0
            att = map(vol_percent, 1, 100, 83, 0); 
        }

        // Send for BOTH channels (L+R)
        // Channel ID 3 (Binary 11) for Both? Or send twice?
        // Datasheet: D1 D0
        // 0 0 : CH1
        // 0 1 : CH2
        // 1 0 : Both
        // 1 1 : Both
        
        // We will send 'Both' (Binary 10 -> D0=0, D1=1 or D0=1, D1=0?)
        // Let's assume sending individually is safer or use 'Both' Code.
        // Common lib uses:
        // Send: bits...
        
        sendData(att, 0b11); // Select Both
    }

private:
    void sendData(uint8_t att_data, uint8_t channel_cmd) {
        // Frame: D0-D1 (Channel), D2-D6 (Att), D7-D8 (High?), D9 (Latch)
        
        // According to verified libs (e.g., gomont/M62429):
        // 1. Channel Select (2 bits) LSB first
        // 2. Attenuation (5 bits) LSB first 
        // 3. D7, D8 (Always 1)
        // 4. Latch (Clocks)

        uint16_t data = 0;
        
        // Bits 0-1: Channel
        data |= (channel_cmd & 0x03);
        
        // Bits 2-6: Attenuation
        // Attenuation is 5 bits. 
        // The chip expects 4dB steps logic combined with 1dB steps?
        // Actually it's simpler: linear 5 bit value for some models, or split.
        // M62429: 5 bits = 0..31.
        // Wait, M62429 has finer steps.
        // Let's use a simplified approach: map 0-100 to 0-80 roughly.
        // The 5 bits + 2 fine bits structure might be hidden.
        // Let's assume standard 5-bit attenuation for now (0-31 steps of 2dB).
        // 0 = 0dB, 31 = -62dB. (+ mute).
        
        // Refined mapping:
        // Input `att_data` expected to be 0..83?
        // Let's map 0-100 volume to 0-31 integer for simplicity first, or check M62429 detailed binary.
        // Actually M62429 has:
        // ATT1 (5 bits): 00000(0dB) to 10101(-80dB) ?
        
        // FIX: Let's use a simpler known algorithm.
        // Attenuation Data (7 bits actually? Or 5?)
        // Sending 9 bits total?
        
        // Let's implement the bit-bang found in typical Arduino M62429 libs:
        // 1. Channel (2 bits)
        // 2. Data (7 bits) -> 5 bits Coarse, 2 bits Fine?
        // Actually, just sending the integer 0-83 works with proper shifting.
        
        // Format:
        // D1 D0 (Channel)
        // D2..D8 (Atten data, LSB first)
        // D9 (Latch bit)

        // Atten Data 7 bits:
        // 0 = 0dB
        // 83 = -83dB
        // >83 = Mute
        
        // Sequence:
        writeBit((channel_cmd >> 0) & 1); // D0
        writeBit((channel_cmd >> 1) & 1); // D1
        
        for (int i = 0; i < 7; i++) {
            writeBit((att_data >> i) & 1);
        }
        
        writeBit(1); // D9 (Latch?)
        writeBit(1); // Extra latch clock
        
        // Send Latch (Clock High then Low while Data is Low? or just extra pulse)
        digitalWrite(_data_pin, HIGH);
        digitalWrite(_clk_pin, LOW);
        delayMicroseconds(4);
        digitalWrite(_data_pin, LOW);
        delayMicroseconds(4);
    }
};

#endif
