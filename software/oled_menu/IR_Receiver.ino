// #if FLASHEND >= 0x3FFF  // For 16k flash or more, like ATtiny1604. Code does not fit in program memory of ATtiny85 etc.
// // !!! Enabling B&O disables detection of Sony, because the repeat gap for SONY is smaller than the B&O frame gap :-( !!!
// //#define DECODE_BEO // Bang & Olufsen protocol always must be enabled explicitly. It has an IR transmit frequency of 455 kHz! It prevents decoding of SONY!
// #endif

// #if !defined(RAW_BUFFER_LENGTH)
// // For air condition remotes it requires 750. Default is 200.
// #  if !((defined(RAMEND) && RAMEND <= 0x4FF) || (defined(RAMSIZE) && RAMSIZE < 0x4FF))
// #define RAW_BUFFER_LENGTH  730 // this allows usage of 16 bit raw buffer, for RECORD_GAP_MICROS > 20000
// #  endif
// #endif

// #if defined(DECODE_BEO)
// #define RECORD_GAP_MICROS 16000 // always get the complete frame in the receive buffer, but this prevents decoding of SONY!
// #endif

// #include <IRremote.hpp>

// #if defined(APPLICATION_PIN)
// #define DEBUG_BUTTON_PIN    APPLICATION_PIN // if low, print timing for each received data set
// #else
// #define DEBUG_BUTTON_PIN   6
// #endif

// void generateTone();
// void handleOverflow();
// bool detectLongPress(uint16_t aLongPressDurationMillis);