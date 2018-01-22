/**
 * @file multigrain.ino
 * @author Alex Hiam - <alex@graycat.io>
 * @copyright Copyright (c) 2018 - Gray Cat Labs, Alex Hiam <alex@graycat.io>
 * @license Released under the MIT License
 * 
 * @brief An alternate firmware for the GinkoSynthese Grains eurorack module.
 * Grains: https://www.ginkosynthese.com/product/grains/
 */

#include "grains.h"

#define MODE_KNOB KNOB_1

#define CLOCK_GEN_MIN_PERIOD_US 1000
#define CLOCK_GEN_MAX_PERIOD_US 2000000

#define T_TO_G_MIN_WIDTH_US     0
#define T_TO_G_MAX_WIDTH_US     2000000

/**
 * The different modes (states)
 */
typedef enum {
  MODE_CLOCK_GEN=0, ///< Clock generator
  MODE_T_TO_G,      ///< Trigger to gate converter
  MODE_NOISERING,   ///< "Noisering"
  
  N_MODES           ///< Number of modes
} mode_t;

// The next value to be written to the PWM OCR register:
static volatile uint8_t sg_pwm_value = 0;

static mode_t sg_current_mode = MODE_CLOCK_GEN;

/**
 * Samples the mode input and returns the currently selected mode.
 */
mode_t getMode() {
  uint16_t mode_val;
  static avg_t mode_avg;

  // Read the mode knob:
  mode_val = sampleAveraged(MODE_KNOB, &mode_avg);

  if (mode_val < 128) {
    // First 1/4
    return MODE_CLOCK_GEN;
  }
  else if (mode_val < 512) {
    // Second 1/4
    return MODE_T_TO_G;
  }
  else {
    // Second 1/2 of the knob is all noisering
    return MODE_NOISERING;
  }
}

void setup() {
  pinMode(PWM_PIN, OUTPUT);
  audioOn();
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  mode_t mode;

  // Get the selected mode:
  mode = getMode();

  // Run the non-interrupt part of the modes:
  switch (mode) {
  case MODE_CLOCK_GEN:
    runClockGen();
    break;
    
  case MODE_T_TO_G:
    runTtoG();
    break;

  case MODE_NOISERING:
    runNoiseRing();
    break;
       
  default:
    // Shouldn't ever happen
    break;  
  }
  
  // We set the static global mode variable after running the mode
  // routines, when there's a mode transition it's garunteed that
  // the mode has been 'setup' before the interrupt part is run:
  sg_current_mode = mode;
}



//=====clock generator===========================

static uint64_t sg_clock_gen_on_us = 0;
static uint64_t sg_clock_gen_off_us;

/**
 * To be run in the main loop
 */
void runClockGen() {
  static avg_t freq_avg, duty_avg;
  static uint16_t last_freq=1025, last_duty=1025;
  uint16_t freq, duty;
  uint64_t period_us, on_us;

  // Use a logaithmic response for the frequency knob for better control:
  freq = mapLog(sampleAveraged(KNOB_2, &freq_avg));

  duty = sampleAveraged(KNOB_3, &duty_avg);

  if (freq != last_freq || duty != last_duty) {
    // Only calculate/update values if changed

    last_duty = duty;
    last_freq = freq;
    
    duty = map(duty, 0, 1023, 0, 110); // max of 110 gives some dead space at the top to ensure100%
    duty = constrain(duty, 0, 100);
  
    
    period_us = map(freq, 0, 1023, CLOCK_GEN_MAX_PERIOD_US, CLOCK_GEN_MIN_PERIOD_US);
    // Just to be safe:
    period_us = constrain(period_us, CLOCK_GEN_MIN_PERIOD_US, CLOCK_GEN_MAX_PERIOD_US);
  
    
    // Clear on-time (disable output) before changing to reduce glitching:
    sg_clock_gen_on_us = 0;
    
    on_us = (period_us / 100) * duty;
    sg_clock_gen_off_us = period_us - on_us;
    sg_clock_gen_on_us = on_us;
  }
}

/**
 * To be run in the PWM ISR
 */
static void tickClockGen() {
  static uint64_t last_transition_us=0;
  static uint8_t state;
  uint64_t t_us, elapsed, target;

  // Get current time:
  t_us = micros();
  elapsed = t_us - last_transition_us;

  if (sg_clock_gen_on_us) {
    // Non-zero on-time
    
    if (sg_clock_gen_off_us == 0) {
      // 100% duty cycle
      state = 0xff;
    }
    else {
      // Get width of current phase:
      target = state ? sg_clock_gen_on_us : sg_clock_gen_off_us;

      if (elapsed >= target) {
        // Width reached, transition
        last_transition_us = t_us;
        state ^= 0xff;
      }
    }    
  } 
  else {
    // 0% duty cycle, disable output
    state = 0;
  }
  sg_pwm_value = state;
  digitalWrite(LED_PIN, state & 0x1);
}
//===============================================



//=====trigger to gate===========================

static uint64_t sg_t_to_g_width_us = 0;
static uint64_t sg_t_to_g_trigger_us = 0;


void runTtoG() {
  static avg_t width_avg, prob_avg;
  uint64_t pulse_width;
  uint16_t prob;

  pulse_width = mapExp(sampleAveraged(KNOB_3, &width_avg));

  pulse_width = map(pulse_width, 0, 1023, T_TO_G_MIN_WIDTH_US, T_TO_G_MAX_WIDTH_US);
  // Just to be safe:
  pulse_width = constrain(pulse_width, T_TO_G_MIN_WIDTH_US, T_TO_G_MAX_WIDTH_US);

  
  // Probability to skip a trigger:
  prob = mapExp(sampleAveraged(KNOB_2, &prob_avg));
  prob = map(prob, 0, 1023, 0, 100);
  
  if (getTrigger(EDGE_RISING)) {
    // WeTrigger received
    if (prob < 100) {
      if (prob == 0 || !random(prob)) {
        sg_t_to_g_width_us = pulse_width;
        sg_t_to_g_trigger_us = micros();
        sg_pwm_value = 0xff;
        digitalWrite(LED_PIN, 0x1);
      }
    }
    // Otherwise skip at 100%, nothing to do
    
  }
  else {
    // No new trigger - only update pulse_width if currently gating:
    if (sg_pwm_value) {
      sg_t_to_g_width_us = pulse_width;
    }
    // Otherwise it can falsely "retrigger" when increasing pulse width

    else {
      digitalWrite(LED_PIN, 0x0);      
    }
  }
}

static inline void tickTtoG() {
  uint64_t t_us, elapsed, target;
  // Current time:
  t_us = micros();
  
  elapsed = t_us - sg_t_to_g_trigger_us;
  
  if (sg_t_to_g_width_us) {
    // Non-zero pulse width
    if (elapsed >= sg_t_to_g_width_us) {
      // Pulse over
      sg_pwm_value = 0;
    }
    else {
      sg_pwm_value = 0xff;
    }
  }
  else {
    sg_pwm_value = 0;
  }
}


//===============================================



//=====noisering=================================

// Initial value from grainsring:
static uint16_t sg_noise_ring_bit_array = 0x8080;
static uint8_t sg_noise_ring_bits_to_write = 1;

void runNoiseRing() {
  uint16_t new_bits;
  uint8_t change, chance, next_value;
  static avg_t bits_avg;
  uint16_t bits;

  // Read the mode knob:
  bits = sampleAveraged(MODE_KNOB, &bits_avg);
  bits = constrain(bits, 512, 1000);
  
  sg_noise_ring_bits_to_write = map(bits, 512, 1000, 8, 1);
  
  // Probabilities based on knob values
  change = analogRead(KNOB_2) > random(1024);
  chance = analogRead(KNOB_3) > random(1024);

    if (getTrigger(EDGE_RISING)) {
      // Rotate the bit array
      next_value = bitRead(sg_noise_ring_bit_array, 15);
      new_bits = sg_noise_ring_bit_array << 1;
      if (change) {
        next_value = chance;
      }
      bitWrite(new_bits, 0, next_value);
      sg_noise_ring_bit_array = new_bits;
      digitalWrite(LED_PIN, new_bits & 0x1);
    }
}

static inline void tickNoiseRing() {
  uint8_t mask = 0xff;
  if (sg_noise_ring_bits_to_write == 1) {
    sg_pwm_value = (sg_noise_ring_bit_array & 0x1) * 0xff;
  }
  else {
    mask >>= 8 - sg_noise_ring_bits_to_write;
    sg_pwm_value = sg_noise_ring_bit_array & mask;
  }
}

//===============================================


// PWM ISR
SIGNAL(PWM_INTERRUPT) {
  switch (sg_current_mode) {
    
  case MODE_CLOCK_GEN:
    tickClockGen();
    break;
    
  case MODE_T_TO_G:
    tickTtoG();
    break;

  case MODE_NOISERING:
    tickNoiseRing();
    break;

  default:
    // This should never happen, but just in case:
    sg_pwm_value = 0;
    break;  
  }

  // Update the PWM output:
  setPwm(sg_pwm_value);
}


