/**
 * @file grains.h
 * @author Alex Hiam - <alex@graycat.io>
 * @copyright Copyright (c) 2018 - Gray Cat Labs, Alex Hiam <alex@graycat.io>
 * @license Released under the MIT License
 * 
 * @brief A basic C library for the Ginko Synthese Grains eurorack module.
 * Grains: https://www.ginkosynthese.com/product/grains/
 */


#ifndef _GRAINS_H
#define _GRAINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// I/O defines:
#define KNOB_1  2
#define KNOB_2  1
#define KNOB_3  0
#define CV_IN   3

#define PWM_PIN       11
#define PWM_OCR       (OCR2A)
#define PWM_INTERRUPT (TIMER2_OVF_vect)

#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5


#define SAMPLE_AVG_LEN 4 ///< Moving average length for analog input sampling:


/**
 * Struct for use with #sampleAveraged function.
 */
typedef struct {
  uint16_t window[SAMPLE_AVG_LEN];
  uint32_t sum;
  uint8_t i;
} avg_t;


inline void setPwm(uint8_t value) { 
  PWM_OCR = value;
}

extern const uint16_t freqTable[];
extern const uint16_t expTable[];
extern const uint16_t logTable[];
extern const uint16_t semitoneTable[];
extern const uint16_t majorTable[];

/**
 * Convert CV ADC value to frequency
 */
inline uint16_t mapFreq(uint16_t input) {
  return pgm_read_word_near(freqTable + input);
}

/**
 * Convert linear ADC input to exponential
 * 
 * (Finer control on ends)
 */
inline uint16_t mapExp(uint16_t input) {
  return pgm_read_word_near(expTable + input);
}

/**
 * Convert linear ADC input to logarithmic
 *
 * (Finer control in the middle)
*/
inline uint16_t mapLog(uint16_t input) {
  return pgm_read_word_near(logTable + input);
}

inline uint16_t mapSemitone(uint16_t input) {
  return pgm_read_word_near(semitoneTable + input);
}

inline uint16_t mapMajor(uint16_t input) {
  return pgm_read_word_near(majorTable + input);
}

/**
 * Enalbe the PWM output
 */
void audioOn(void);

/**
 * Get the current state of a clock signal on IN1
 */
uint8_t getClock(void);


/**
 * Used with #getTrigger to spedify which edge to look for
 */
typedef enum {
  EDGE_FALLING=0,
  EDGE_RISING
} edge_t;

/**
 * Returns 1 if a trigger has been received since the last time
 * called, 0 otherwise.
 */
uint8_t getTrigger(edge_t edge);

/**
 * Sample the given ADC input and return the output of a moving
 * average. (Each input  needs its own static avg_t struct)
 */
uint16_t sampleAveraged(uint8_t input, avg_t *avg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //_GRAINS_H

