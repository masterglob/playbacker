#ifndef SLAVE_CONSTS_INC
#define SLAVE_CONSTS_INC
/*
 * 
 * GLOBAL PINOUT

 TX = MIDI OUT (MIDI #5)(via R220)
 RX = I2S DATA
 D2  = Serial IN (from RPI3)
 D4 = I2S LRCK
 D7 = Input test btn
 D8 = I2S BCK

 GND => I2S (*2) + RPI3 + MIDI #2, Ext_Alim
 +5V => MIDI #4 (via R220), I2S, Ext_Alim

 Ext_Alim required, otherwise there may be not enough power from USB.
 
 
 */
/**
 * Sine test duration (seconds)
 */
#define DEBUG_LRC_SINE_TEST 2

/**
 * I2S output frequency: 44100 or 22050
 * - RX = DATA /SD (GPIO3)
 * - D4 = LRCK /WS (GPIO2)
 * - D8 = BCK / SCK (GPIO15)
 */
#define I2S_HZ_FREQ (22050)
#define I2S_HZ_FREQ_DIV (44100 / I2S_HZ_FREQ) // DO NOT EDIT
#define I2S_PIN_DATA 3 
#define I2S_PIN_LRCK 2
#define I2S_PIN_BCK 15 

/** 
 *  Overclocking 80 -> 160 MHz
 *  set to 0 or 1
 */
#define OVERCLOCK (1)

/**
 * Debug serial line
 */
#define SERIAL_BAUDRATE (115200)

/**
 * Software serial. Used to receive commands frm MASTER
 */
#define SOFT_SER_RX (D2)
#define SOFT_SER_TX (D3)
#define RX_EXP_BAUDRATE (115200)
#define RX_PRG_BAUDRATE (115200 /(1+OVERCLOCK)) // DO NOT EDIT

/*
 * External button.
 * launches 2s sine test when pressed
 */
#define BTN_TEST (D7)

/**
 * MIDI config
 * MIDI_CHANNEL = 1 .. 16
 * MIDI_BAUD_RATENEL = 31250
 * MIDI pinout = TX (GPIO1)
 */
#define MIDI_CHANNEL (16)
#define MIDI_BAUD_RATE (31250)

const int16_t LRC_wavSine[256] = {
    0x0000, 0x0324, 0x0647, 0x096a, 0x0c8b, 0x0fab, 0x12c8, 0x15e2,
    0x18f8, 0x1c0b, 0x1f19, 0x2223, 0x2528, 0x2826, 0x2b1f, 0x2e11,
    0x30fb, 0x33de, 0x36ba, 0x398c, 0x3c56, 0x3f17, 0x41ce, 0x447a,
    0x471c, 0x49b4, 0x4c3f, 0x4ebf, 0x5133, 0x539b, 0x55f5, 0x5842,
    0x5a82, 0x5cb4, 0x5ed7, 0x60ec, 0x62f2, 0x64e8, 0x66cf, 0x68a6,
    0x6a6d, 0x6c24, 0x6dca, 0x6f5f, 0x70e2, 0x7255, 0x73b5, 0x7504,
    0x7641, 0x776c, 0x7884, 0x798a, 0x7a7d, 0x7b5d, 0x7c29, 0x7ce3,
    0x7d8a, 0x7e1d, 0x7e9d, 0x7f09, 0x7f62, 0x7fa7, 0x7fd8, 0x7ff6,
    0x7fff, 0x7ff6, 0x7fd8, 0x7fa7, 0x7f62, 0x7f09, 0x7e9d, 0x7e1d,
    0x7d8a, 0x7ce3, 0x7c29, 0x7b5d, 0x7a7d, 0x798a, 0x7884, 0x776c,
    0x7641, 0x7504, 0x73b5, 0x7255, 0x70e2, 0x6f5f, 0x6dca, 0x6c24,
    0x6a6d, 0x68a6, 0x66cf, 0x64e8, 0x62f2, 0x60ec, 0x5ed7, 0x5cb4,
    0x5a82, 0x5842, 0x55f5, 0x539b, 0x5133, 0x4ebf, 0x4c3f, 0x49b4,
    0x471c, 0x447a, 0x41ce, 0x3f17, 0x3c56, 0x398c, 0x36ba, 0x33de,
    0x30fb, 0x2e11, 0x2b1f, 0x2826, 0x2528, 0x2223, 0x1f19, 0x1c0b,
    0x18f8, 0x15e2, 0x12c8, 0x0fab, 0x0c8b, 0x096a, 0x0647, 0x0324,
    0x0000, 0xfcdc, 0xf9b9, 0xf696, 0xf375, 0xf055, 0xed38, 0xea1e,
    0xe708, 0xe3f5, 0xe0e7, 0xdddd, 0xdad8, 0xd7da, 0xd4e1, 0xd1ef,
    0xcf05, 0xcc22, 0xc946, 0xc674, 0xc3aa, 0xc0e9, 0xbe32, 0xbb86,
    0xb8e4, 0xb64c, 0xb3c1, 0xb141, 0xaecd, 0xac65, 0xaa0b, 0xa7be,
    0xa57e, 0xa34c, 0xa129, 0x9f14, 0x9d0e, 0x9b18, 0x9931, 0x975a,
    0x9593, 0x93dc, 0x9236, 0x90a1, 0x8f1e, 0x8dab, 0x8c4b, 0x8afc,
    0x89bf, 0x8894, 0x877c, 0x8676, 0x8583, 0x84a3, 0x83d7, 0x831d,
    0x8276, 0x81e3, 0x8163, 0x80f7, 0x809e, 0x8059, 0x8028, 0x800a,
    0x8000, 0x800a, 0x8028, 0x8059, 0x809e, 0x80f7, 0x8163, 0x81e3,
    0x8276, 0x831d, 0x83d7, 0x84a3, 0x8583, 0x8676, 0x877c, 0x8894,
    0x89bf, 0x8afc, 0x8c4b, 0x8dab, 0x8f1e, 0x90a1, 0x9236, 0x93dc,
    0x9593, 0x975a, 0x9931, 0x9b18, 0x9d0e, 0x9f14, 0xa129, 0xa34c,
    0xa57e, 0xa7be, 0xaa0b, 0xac65, 0xaecd, 0xb141, 0xb3c1, 0xb64c,
    0xb8e4, 0xbb86, 0xbe32, 0xc0e9, 0xc3aa, 0xc674, 0xc946, 0xcc22,
    0xcf05, 0xd1ef, 0xd4e1, 0xd7da, 0xdad8, 0xdddd, 0xe0e7, 0xe3f5,
    0xe708, 0xea1e, 0xed38, 0xf055, 0xf375, 0xf696, 0xf9b9, 0xfcdc,
};
const int16_t SAMPLES_Clic1[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x004E, 0x0029, 0xFFE9, 0x002F, 0xFFE4, 0xFFF8, 0xFF91, 0xFFBA, 0xFFF5, 0x0022, 0x0038,
    0x0052, 0x0043, 0x0005, 0x870F, 0x8663, 0x86D3, 0x869D, 0x86EF, 0x8691, 0x8714, 0x870E, 0x8712, 0x86A5, 0x86CA, 0x8676, 0x8698,
    0x8638, 0x863E, 0x869C, 0x8657, 0x86F6, 0x864B, 0x869C, 0x86D2, 0x86E6, 0x86E1, 0x8763, 0x8715, 0x871B, 0x863B, 0x87B1, 0x869A,
    0x8754, 0x8731, 0x8764, 0x86B2, 0x86FA, 0x8697, 0x870B, 0x8643, 0x86A9, 0x8641, 0x861D, 0x86EF, 0x8675, 0x872C, 0x86C7, 0x862B,
    0x8728, 0x85EA, 0x875E, 0x86B8, 0x870D, 0x6025, 0x60AD, 0x6104, 0x60DE, 0x60E9, 0x614A, 0x613A, 0x60C6, 0x6075, 0x5FE4, 0x611A,
    0x605F, 0x6108, 0x60C8, 0x6089, 0x6106, 0x60B8, 0x60D4, 0x612C, 0x60B9, 0x6175, 0x606D, 0x60FC, 0x6064, 0x60DF, 0x606D, 0x6077,
    0x6021, 0x611B, 0x6081, 0x60D7, 0x60AC, 0x60F7, 0x614C, 0x60F7, 0x611C, 0x615E, 0x610D, 0x6089, 0x61A0, 0x613C, 0x60D0, 0x60FB,
    0x6029, 0x61BC, 0x6111, 0x60F6, 0x6068, 0x6048, 0x60B9, 0xB2CA, 0xB283, 0xB2C3, 0xB2C7, 0xB28E, 0xB275, 0xB2C3, 0xB2A5, 0xB2DD,
    0xB30D, 0xB1DE, 0xB2C6, 0xB20B, 0xB26F, 0xB2F4, 0xB220, 0xB318, 0xB27E, 0xB2ED, 0xB373, 0xB21C, 0xB2E3, 0xB20D, 0xB237, 0xB2C6,
    0xB262, 0xB328, 0xB2CA, 0xB2B5, 0xB245, 0xB2AC, 0xB252, 0xB308, 0xB21D, 0xB234, 0xB23B, 0xB211, 0xB294, 0xB242, 0xB2A3, 0xB162,
    0xB340, 0xB234, 0xB330, 0xB212, 0xB28E, 0xB2B6, 0xB34B, 0xB2BF, 0xB2B7, 0x3D75, 0x3D56, 0x3DBA, 0x3D7E, 0x3E1D, 0x3EB0, 0x3DFC,
    0x3DD4, 0x3E1E, 0x3DA9, 0x3E7F, 0x3D9F, 0x3DDF, 0x3E7A, 0x3E54, 0x3E11, 0x3DD0, 0x3E32, 0x3E04, 0x3D76, 0x3DB9, 0x3DC7, 0x3E26,
    0x3E2B, 0x3E24, 0x3E34, 0x3E36, 0x3E06, 0x3E17, 0x3DEE, 0x3DE6, 0x3E7A, 0x3CF7, 0x3E7E, 0x3D32, 0x3D56, 0x3DD6, 0x3D46, 0x3E20,
    0x3DD2, 0x3D9A, 0x3DE7, 0x3D7D, 0x3DD1, 0x3E84, 0x3D8D, 0x3EBA, 0x3D2A, 0x3DBA, 0x3DE4, 0xCF0E, 0xCE99, 0xCF10, 0xCED3, 0xCE72,
    0xCE33, 0xCE2F, 0xCEAC, 0xCE04, 0xCEF7, 0xCE02, 0xCF01, 0xCEA4, 0xCE88, 0xCE9B, 0xCEC7, 0xCED4, 0xCE93, 0xCE34, 0xCE63, 0xCE75,
    0xCEDE, 0xCE53, 0xCEB4, 0xCE78, 0xCE0E, 0xCEF1, 0xCDE6, 0xCEAE, 0xCE91, 0xCE2F, 0xCECD, 0xCDB4, 0xCEFF, 0xCE7D, 0xCF6A, 0xCF47,
    0xCEC6, 0xCEE0, 0xCE2C, 0xCEED, 0xCE84, 0xCEA6, 0xCEC2, 0xCE3D, 0xCEBE, 0xCE65, 0xCEAF, 0xCE88, 0xCE9F, 0x26C5, 0x27C3, 0x2679,
    0x278E, 0x2681, 0x27D3, 0x273F, 0x2770, 0x282A, 0x2746, 0x277A, 0x271C, 0x2624, 0x27AF, 0x2684, 0x2777, 0x2778, 0x27A8, 0x284C,
    0x27D7, 0x27BB, 0x279F, 0x275A, 0x2727, 0x275B, 0x27EE, 0x2798, 0x27E1, 0x27E1, 0x26DC, 0x281F, 0x26A9, 0x2821, 0x2741, 0x277A,
    0x270C, 0x26F5, 0x2774, 0x273C, 0x278F, 0x26FB, 0x277B, 0x26D7, 0x274F, 0x273C, 0x26E8, 0x2795, 0x2705, 0x27C2, 0x2769, 0xE05B,
    0xE017, 0xE03F, 0xDFDC, 0xE117, 0xDFEE, 0xE03C, 0xE078, 0xE03C, 0xE042, 0xE099, 0xE04C, 0xE113, 0xE045, 0xE04B, 0xE093, 0xDFD8,
    0xE09C, 0xDFFD, 0xDFDB, 0xE053, 0xDFBC, 0xE05D, 0xE003, 0xE047, 0xDFC4, 0xE051, 0xDFFD, 0xE0A8, 0xE0A1, 0xDFF6, 0xE01C, 0xE04C,
    0xE07D, 0xE10E, 0xE14C, 0xE0D1, 0xE080, 0xE0B4, 0xE0A5, 0xE024, 0xDFCD, 0xE083, 0xDF60, 0xE004, 0xE041, 0xE03A, 0xE0C6, 0xE0A1,
    0xE017, 0x196B, 0x18A0, 0x1944, 0x18FF, 0x18A2, 0x1941, 0x18D5, 0x1961, 0x18ED, 0x1924, 0x1977, 0x197B, 0x18E4, 0x194E, 0x196A,
    0x18FB, 0x1995, 0x18AC, 0x19D7, 0x193E, 0x1987, 0x198C, 0x193C, 0x19FB, 0x18B5, 0x1A43, 0x1982, 0x19F0, 0x1953, 0x199E, 0x199D,
    0x19A5, 0x198F, 0x19F0, 0x1920, 0x18BC, 0x18DB, 0x18BF, 0x1961, 0x1918, 0x19C6, 0x199D, 0x1963, 0x19A7, 0x1933, 0x197F, 0x1967,
    0x1910, 0x18E9, 0x194F, 0xEBE4, 0xECCB, 0xEBB9, 0xEC67, 0xEB69, 0xEBBB, 0xEBFE, 0xEBAF, 0xEC27, 0xEB85, 0xEB7C, 0xEB92, 0xEC11,
    0xEC01, 0xEBE0, 0xEBE0, 0xEB8C, 0xEC2C, 0xEBB1, 0xEC1F, 0xEBF3, 0xEBFC, 0xEBDB, 0xEBC3, 0xEBE4, 0xEBA1, 0xEB6F, 0xEADD, 0xEB62,
    0xEB50, 0xEBCE, 0xEBC2, 0xEB86, 0xEB87, 0xEBD4, 0xEBF1, 0xEC1C, 0xEBCD, 0xEBB0, 0xEBED, 0xEB2C, 0xEBBB, 0xEBC0, 0xEBC1, 0xEBE4,
    0xEBEE, 0xEB7A, 0xEC3A, 0xEC3F, 0xEBF2, 0x0FFE, 0x0F96, 0x1023, 0x0FD5, 0x0FFA, 0x104C, 0x1056, 0x104E, 0x103C, 0x10AC, 0x1066,
    0x108E, 0x103B, 0x10A0, 0x0FDF, 0x1043, 0x0FD7, 0x1067, 0x1006, 0x109C, 0x0FEC, 0x1094, 0x1050, 0x10D5, 0x0FFF, 0x1064, 0x105F,
    0x0FFF, 0x10DA, 0x0FFC, 0x10AB, 0x108B, 0x0FE2, 0x104C, 0x1008, 0x0FFF, 0x103C, 0x0FDD, 0x106F, 0x0FDE, 0x1026, 0x0FD7, 0x0FF4,
    0x0FCD, 0x1005, 0x0F75, 0x0FF5, 0x0FB5, 0x0FE3, 0x0F7B, 0xF3BF, 0xF33E, 0xF37B, 0xF343, 0xF358, 0xF337, 0xF369, 0xF33A, 0xF3D3,
    0xF2F2, 0xF356, 0xF356, 0xFFCA, 0xFFFA, 0xFFF9, 0xFFB3, 0x002E, 0xFFB7, 0xFFFC, 0xFF8A, 0x0054, 0xFFA8, 0x003F, 0xFFAE, 0x0069,
    0x003A, 0x002E, 0x0055, 0x002A, 0x0057, 0x0023, 0xFFED, 0xFFEF, 0x0020, 0xFF4B, 0x000A, 0xFF60, 0xFFD9, 0x0000, 0x006E, 0x0032,
    0x00E0, 0x0033, 0x008C, 0x001F, 0x0058, 0x0071, 0x0047, 0x0000, 0x0000, 0x0000, 0x0000,
};

const int16_t SAMPLES_Clic2[] = {
    0x0056, 0xFFE3, 0xFFF5, 0x0037, 0x93D9, 0x954A, 0x9416, 0x94DB, 0x948C, 0x94B7, 0x9535, 0x9576, 0x94D2, 0x948C, 0x94A1, 0x947B,
    0x94A7, 0x9456, 0x94C5, 0x9470, 0x9419, 0x94FA, 0x949D, 0x9488, 0x941A, 0x942D, 0x944E, 0x9496, 0x94BE, 0x9467, 0x9491, 0x9482,
    0x94AB, 0x94C8, 0x9422, 0x94DE, 0x9401, 0x949D, 0x947A, 0x9470, 0x94F4, 0x9457, 0x94A1, 0x94C9, 0x93FF, 0x9572, 0x9436, 0x958C,
    0x94BE, 0x944C, 0x949E, 0x93D2, 0x9579, 0x949B, 0x94BC, 0x94FC, 0x940D, 0x9536, 0x9438, 0x9425, 0x9549, 0x93E2, 0x9412, 0x9325,
    0x938F, 0x93FF, 0x941E, 0x93EB, 0x9448, 0x9452, 0x9499, 0x9414, 0x9436, 0x93D2, 0x951C, 0x944E, 0x94F3, 0x94EC, 0x94DA, 0x951B,
    0x9463, 0x955C, 0x9531, 0x94E4, 0x9415, 0x94AC, 0x94B3, 0x948B, 0x94D7, 0x94BF, 0x94AB, 0x9505, 0x9434, 0x9594, 0x9484, 0x94BF,
    0x94E7, 0x95A9, 0x9418, 0x9618, 0x9434, 0x9586, 0x949B, 0x950D, 0x5590, 0x5585, 0x55BA, 0x5578, 0x561B, 0x55F0, 0x5620, 0x55AB,
    0x55F5, 0x5642, 0x5619, 0x55BB, 0x5626, 0x55BC, 0x55EC, 0x5503, 0x55F1, 0x5531, 0x564E, 0x55EC, 0x5632, 0x55D3, 0x5594, 0x5623,
    0x55DD, 0x55FC, 0x5576, 0x55D7, 0x565B, 0x56CC, 0x55ED, 0x55E8, 0x5610, 0x555B, 0x5652, 0x561A, 0x562F, 0x5618, 0x55C6, 0x56B3,
    0x565B, 0x560C, 0x563D, 0x5614, 0x5653, 0x55CD, 0x5643, 0x560F, 0x55B1, 0x55EF, 0x5671, 0x5654, 0x569F, 0x55CF, 0x563C, 0x55ED,
    0x550B, 0x55E5, 0x5580, 0x5690, 0x5656, 0x5611, 0x5688, 0x54C3, 0x5626, 0x5582, 0x561B, 0x55A2, 0x5639, 0x5613, 0x55CE, 0x5637,
    0x557F, 0x5640, 0x555D, 0x5689, 0x5537, 0x5633, 0x55CC, 0x550D, 0x55AD, 0x5522, 0x55E6, 0x55E8, 0x5634, 0x5641, 0x5617, 0x559D,
    0x55FF, 0x55E2, 0x55EC, 0x55B3, 0x55E2, 0x55B7, 0x56E4, 0x55B8, 0x5679, 0x55D5, 0x564E, 0x55DD, 0xBC30, 0xBB39, 0xBBDE, 0xBB69,
    0xBAB6, 0xBAF0, 0xBAB2, 0xBB23, 0xBADE, 0xB9FA, 0xBB5D, 0xBA39, 0xBB72, 0xBBB9, 0xBBAE, 0xBC01, 0xBB72, 0xBBCB, 0xBC24, 0xBC0E,
    0xBBF6, 0xBBEF, 0xBB8F, 0xBBC5, 0xBAB1, 0xBC63, 0xBB2F, 0xBBA2, 0xBBC5, 0xBB72, 0xBB8E, 0xBB07, 0xBB5E, 0xBB37, 0xBC2C, 0xBBE1,
    0xBADC, 0xBBBE, 0xBA41, 0xBB24, 0xBAF0, 0xBB23, 0xBB66, 0xBA94, 0xBA7C, 0xBBBE, 0xBA4C, 0xBADD, 0xBB30, 0xBAA1, 0xBB30, 0xBB6F,
    0xBACA, 0xBAEC, 0xBA90, 0xBB3B, 0xBB84, 0xBAFB, 0xBB4A, 0xBB19, 0xBAE1, 0xBADC, 0xBB06, 0xBB09, 0xBB8E, 0xBB39, 0xBB28, 0xBBE4,
    0xBAAD, 0xBB98, 0xBAC4, 0xBB2B, 0xBB69, 0xBAFC, 0xBB72, 0xBB45, 0xBB96, 0xBB48, 0xBB67, 0xBB91, 0xBB6B, 0xBB21, 0xBBCA, 0xBADF,
    0xBB8B, 0xBA9E, 0xBB19, 0xBB6F, 0xBACE, 0xBAFB, 0xBAB0, 0xBBBC, 0xBAE0, 0xBB91, 0xBB62, 0xBB42, 0xBB7A, 0xBADD, 0xBB8B, 0xBB82,
    0x36F2, 0x3752, 0x3763, 0x3745, 0x37A1, 0x376F, 0x36CC, 0x3705, 0x35F2, 0x381D, 0x36F4, 0x3785, 0x37E8, 0x36FA, 0x3799, 0x374E,
    0x3706, 0x36E7, 0x36A1, 0x36DA, 0x3728, 0x36E9, 0x371E, 0x3770, 0x37BC, 0x377D, 0x373E, 0x375E, 0x374C, 0x37A7, 0x374D, 0x3738,
    0x36F3, 0x3777, 0x362D, 0x3762, 0x3757, 0x3715, 0x37DC, 0x3716, 0x3786, 0x372C, 0x3756, 0x372C, 0x36A4, 0x3669, 0x3717, 0x36E1,
    0x36B0, 0x3744, 0x36E3, 0x36D2, 0x36CC, 0x36E3, 0x3718, 0x36D5, 0x36F8, 0x3756, 0x3714, 0x372D, 0x3719, 0x378D, 0x36D7, 0x3785,
    0x36F1, 0x3769, 0x370D, 0x36EA, 0x36FE, 0x3662, 0x3754, 0x377B, 0x3783, 0x36FF, 0x374A, 0x3756, 0x36AD, 0x3734, 0x36C1, 0x36EF,
    0x3751, 0x375D, 0x378A, 0x370C, 0x3715, 0x3712, 0x36EF, 0x3757, 0x3731, 0x3728, 0x3758, 0x36D7, 0x3793, 0x3654, 0x376D, 0x364B,
    0x36D9, 0x3725, 0x3703, 0x372C, 0xD3F1, 0xD3FF, 0xD46A, 0xD43D, 0xD3F5, 0xD3F1, 0xD40D, 0xD475, 0xD401, 0xD45F, 0xD3D1, 0xD432,
    0xD389, 0xD3DB, 0xD45A, 0xD399, 0xD3D7, 0xD3A7, 0xD3AA, 0xD3EC, 0xD3DF, 0xD498, 0xD418, 0xD3DD, 0xD38D, 0xD399, 0xD3ED, 0xD400,
    0xD41E, 0xD3C3, 0xD3E1, 0xD398, 0xD41F, 0xD3FF, 0xD3C4, 0xD408, 0xD405, 0xD3F5, 0xD3C3, 0xD429, 0xD3DF, 0xD3DC, 0xD3C2, 0xD407,
    0xD3F2, 0xD3F4, 0xD403, 0xD3B6, 0xD408, 0xD3F6, 0xD428, 0xD3CC, 0xD3C9, 0xD3F7, 0xD422, 0xD421, 0xD459, 0xD440, 0xD434, 0xD466,
    0xD41C, 0xD415, 0xD3BF, 0xD46C, 0xD441, 0xD424, 0xD473, 0xD421, 0xD465, 0xD414, 0xD43A, 0xD3E0, 0xD40C, 0xD421, 0xD3DF, 0xD420,
    0xD3B9, 0xD408, 0xD444, 0xD402, 0xD42F, 0xD44F, 0xD3ED, 0xD435, 0xD40A, 0xD40A, 0xD480, 0xD40D, 0xD476, 0xD41D, 0xD409, 0xD406,
    0xD413, 0xD40F, 0xD486, 0xD3ED, 0xD412, 0xD402, 0xD3FA, 0xD40A, 0x2274, 0x22DE, 0x2299, 0x22BE, 0x2314, 0x2276, 0x22B6, 0x22BA,
    0x230E, 0x230E, 0x2312, 0x22C2, 0x001D, 0x0001, 0xFFA8, 0x0011, 0xFFBC, 0x003B, 0xFFDE, 0x0028, 0x002E, 0x000D, 0x004F, 0x003F,
    0x008F, 0x0043, 0x001D, 0xFFFF, 0x000A, 0x0064, 0xFFC2, 0x002C, 0xFFE5, 0xFFBF, 0xFFF0, 0xFFDB, 0xFFF0, 0xFFFC, 0x001B, 0xFFCA,
    0x0007, 0x001E, 0x0027, 0x0011, 0xFF89, 0x0070, 0xFFEF, 0xFFE4, 0x0012, 0xFFF6,
};
#endif //SLAVE_CONSTS_INC