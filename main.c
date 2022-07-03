// CONFIG
#pragma config WDTE = OFF
#pragma config CP = OFF
#pragma config MCLRE = OFF
// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define _XTAL_FREQ  4000000

#define SAVE_SOME_FLASH 1

/*
 * Encoding:
 * 00 - pause between symbols
 * 01 - pause between words
 * 10 - dot
 * 11 - dash
 * 
 * Dot duration = t, dash = 3t, pause = t
 * pause between symbols = 3t, pause between words = 7t
 * 
 * Color (first 2 bits of the first byte):
 * 01 - green
 * 10 - red
 * 11 - yellow
 * 
 * Example:
 * green --. = 0b10111101
 */

// default pause t, ms
#define MORSE_PAUSE 200

#define MORSE_WORD  0b00000001
#define MORSE_DOT   0b00000010
#define MORSE_DASH  0b00000011

#define MESSAGE_LEN 6 // single message length, bytes
#define MESSAGE_NUM 4 // number of messages

const char msg[MESSAGE_NUM][MESSAGE_LEN] = 
                    {   {0b10101010, 0b10111111, 0b00001010, 0b00000000, 0b00000000, 0b00000000}, // SOS
                        {0b11111001, 0b11100010, 0b10100010, 0b11111000, 0b11001000, 0b00000000}, // HELLO (RUS)
                        {0b10111111, 0b11100011, 0b11110010, 0b10001010, 0b10111110, 0b00000010}, // QRZ?
                        {0b10111011, 0b11110111, 0b10001010, 0b10110011, 0b10111000, 0b00110011},  // I'M BUSY (RUS)
                    };

void main()
{
    OPTION = 0b00000111; // enable wake-up on pin change, enable weak pull-ups, disable T0CKI on GP2
    OSCCALbits.FOSC4=0;  // disable clock output on GP2
    TRISGPIO = 0b1000;   // configure GP3 as input, GP0-GP2 as outputs

    while (1) {
        char counter = 0;
        // 8-bit random number generator
        do {
          counter++;
        } while (!(GPIO & 0b00001000));

        char color = 0;
        char byte = 0;
        for (char k = 0; k < MESSAGE_LEN*8; k += 2) {
            if (k % 8 == 0) {
                // "k >> 3" means "k / 8" optimized
                byte = msg[counter % MESSAGE_NUM][k >> 3];
                if (byte == 0) {
                    break;
                }
            }
            
            char twobits = (byte >> (k % 8)) & 0b00000011;
            
            if (k == 0) {
                color = (char)(twobits << 1);
                continue;
            }

#if SAVE_SOME_FLASH
            if (twobits & MORSE_DOT) {
                GPIO = color;
                __delay_ms(MORSE_PAUSE);
                if (twobits == MORSE_DASH) {
                    __delay_ms(MORSE_PAUSE * 2);
                }
                GPIO = 0;
                __delay_ms(MORSE_PAUSE);
            } else {
                __delay_ms(MORSE_PAUSE * 3);
                if (twobits == MORSE_WORD) {
                    __delay_ms(MORSE_PAUSE * 4);
                }
            }
#else
            // switch-case needs ~10 bytes more
            switch (twobits) {
                case MORSE_DOT:
                    GPIO = color;
                    __delay_ms(MORSE_PAUSE);
                    break;
                case MORSE_DASH:
                    GPIO = color;
                    __delay_ms(MORSE_PAUSE * 3);
                    break;
                case MORSE_WORD:
                    __delay_ms(MORSE_PAUSE * 6);
                    break;
                default:
                    __delay_ms(MORSE_PAUSE * 2);
            }
            GPIO = 0;
            __delay_ms(MORSE_PAUSE);
#endif
        }

        // refresh GPIO input register before sleep by reading it
        counter = GPIO;
        SLEEP();
    }
}