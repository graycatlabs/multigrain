# multigrain

Copyright (c) 2018 - Gray Cat Labs, Alex Hiam <alex@graycat.io>

**multigrain** is an alternate firmware for the Ginkosynthese Grains eurorack module.

## Modes

The top knob (**mode+**) selects the **multigrain** mode from the following options:

### Clock generator

With the **mode+** knob below 25% the **multigrain** acts as a clock generator.

* **Knob 2** : clock frequency (current range ~0.5Hz - 1kHz)
* **Knob 3** : duty cycle (0% - 100%)

The clock can be stopped with the output low by turning the **duty cycle** knob fully CCW (0%), and stopped with the output high by turning the **duty cycle** knob fully CW (100%).

There's currently no CV frequency control but I may add it eventually...

### Trigger to gate converter

With the **mode+** knob between 25% and 50% the **multigrain** acts as a trigger to gate converter, or pulse stretcher.

* **Knob 2** : probability of skipping a trigger
* **Knob 3** : pulse width (0s - 2s)
* **IN 3** : trigger input

The skip probability is inspired by the Erica Synths Pico TG: https://www.perfectcircuitaudio.com/erica-synths-pico-tg-trigger-to-gate-converter.html

Note that if retriggered while the output is high, the pulse width counter is reset without de-asserting the output. So if there's triggers coming in with a period > pulse width the output will remain high.


### Grainsring

With the **mode+** knob above 50% the **multigrain** acts as a modified version of Janne G:son Berg's *grainsring* (available for download on the Grains product page).

* **Knob 1** : (above 50%) bits to output (8 - 1)
* **Knob 2** : "Change": probability that the next new bit will be shifted into the bit sequence
* **Knob 3** : "Chance": probability for the next new bit to be a 1
* **IN 3** : trigger input

The basic idea is that there's a 16-bit value that is being circularly bit shifted at each step (input trigger). During each shift, there is a chance (set by **Change**) that a new bit will be shifted in instead of the previous top bit. **Chance** sets the probability that the new bit will be a 1.

At each trigger, after the bits are shifted, the 8-bit output CV is set to the value of the lowest bytes of the bit array. The **mode+** knob controls the quantization of the output voltage, selecting from the options:

* No quantization
* Quantize to semitones
* Quantize to Major scale
* Quantize to Minor scale
* Quantize to Mixolydian scale

Note that the quantization isn't at all calibrated - all scales are relative to a root of 0V. It's also assumed that the 0-5V output is being used directly. If the output CV is being amplified you'd need to recalculate the lookup tables.


## Installing

**multigrain** can be loaded on your Grains using the Arduino IDE. Just be sure to **not** connect the USB to your computer when the module is powered. See the Grains product page for more info.


## Hardware modifications

Grains was originally intended to output audio, not control voltages/gates, so for the best results you'll need to make a couple modifications:

### Short C6

**C6** (10uF electrolytic) AC couples the PWM output to the **OUT** jack. We want DC coupling to output voltages, so just solder a wire across it.

### Output RC filter

The "analog" output is generated using a PWM signal at 31.25kHz. For a cleaner voltage this needs to be low-pass filtered. There's already a 150ohm series resistor (**R8**), so just adding a 0.1uF ceramic capacitor between the tip and ring connections of the output jack will form an RC low pass filter with a -6dB cutoff of ~10.6kHz.

### Optional: Mix DC input

The maximum voltage the Grains can output is 5V. If you're using the Grains in conjunction with a Ginkosynthese Mix, you can modify it to have a DC coupled input by simply shorting one of the input caps (the big red ones). I shorted my IN1 cap, so channel one is now dedicated to boosting the Grains CV output from 0-5V to 0-10V. And when not using grains I can still use the Mix as a 2-channel audio mixer/amp.

## Credit

Included is a basic Grains C library (`grains.h/.c`), which is based on the original Ginkosynthese Grains firmware, which includes the following credit to Peter Knight:

> // based on the Auduino code by Peter Knight, revised and finetuned by Ginkosynthese for use with cv inputs.


The noise ring mode is based on the original code by Janne G:son Berg.

# License

**multigrain** is released under the MIT license.

```
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
