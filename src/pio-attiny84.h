// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D 10)  PB0  2|    |13  AREF (D  0)
//             (D  9)  PB1  3|    |12  PA1  (D  1)
//                     PB3  4|    |11  PA2  (D  2)
//  PWM  INT0  (D  8)  PB2  5|    |10  PA3  (D  3)
//  PWM        (D  7)  PA7  6|    |9   PA4  (D  4)
//  PWM        (D  6)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+

// Target IC: ATtiny84, internal 8 MHz

#ifndef PIO_ATTINY84_PIN_NAMES
#define PIO_ATTINY84_PIN_NAMES

#define PB0 (0)
#define PB1 (1)
#define PB2 (2)
#define PA7 (3)
#define PA6 (4)
#define PA5 (5)
#define PA4 (6)
#define PA3 (7)
#define PA2 (8)
#define PA1 (9)
#define PA0 (10)

#endif // PIO_ATTINY84_PIN_NAMES
