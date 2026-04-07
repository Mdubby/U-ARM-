/*
 * U-ARM — Upper Arm Rehabilitation Machine
 * EMG Signal Acquisition, Filtering, and Motor Control Firmware
 *
 * Authors: Matthew Wallace, Monte Howell, Jose Moreno Duran, Kirsche Stanton
 * Course:  BME 491 — Bioengineering Design and Analysis
 * School:  University of Nevada, Reno
 * Date:    Spring 2023
 *
 * Description:
 *   Acquires surface EMG signals from the bicep at 1000 Hz via the ATmega2560
 *   ADC, passes the signal through a bandpass + 60 Hz notch filter, computes
 *   the signal envelope via rectification and clamping, applies a calibrated
 *   contraction threshold, and maps the result to a 0–255 PWM output that
 *   drives a gimbal motor providing assisted arm-curl motion for rehabilitation.
 *
 *   The motor control update rate in the integrated U-ARM system runs at 75 Hz.
 *   This firmware module samples EMG at 1000 Hz to ensure sufficient signal
 *   fidelity for the downstream filtering and envelope detection pipeline.
 *
 * Hardware:
 *   - ATmega2560 (Arduino Mega)
 *   - EMG sensor connected to Analog Pin A0
 *   - Gimbal motor / H-bridge connected to Digital Pin 7 (PWM-capable)
 *
 * Calibration:
 *   Before each session:
 *   1. Attach electrodes to patient bicep.
 *   2. Have patient relax fully for several seconds.
 *   3. Observe peak serial envelope value at rest.
 *   4. Set Threshold to ~20% above that resting peak value.
 *   5. Have patient perform a maximum-effort contraction.
 *   6. Note the peak envelope value and update ENVELOPE_MAX accordingly.
 */

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "EMGFilters.h"

// ── Debug ──────────────────────────────────────────────────────────────────
// Set to 1 during development to print envelope values over serial.
// Set to 0 for deployment — serial printing at 1000 Hz adds ~150 µs per loop
// and will reduce effective sample rate below the 1000 Hz target.
#define TIMING_DEBUG 0

// ── Pin Assignments ────────────────────────────────────────────────────────
#define SensorInputPin A0   // EMG sensor signal input
#define OutputPin      7    // PWM output to gimbal motor driver

// ── Filter Configuration ───────────────────────────────────────────────────
// Sample rate must be SAMPLE_FREQ_500HZ or SAMPLE_FREQ_1000HZ.
// Other values bypass all EMG filtering.
int sampleRate = SAMPLE_FREQ_1000HZ;

// Notch filter frequency must match local power grid.
// US power grid: 60 Hz → NOTCH_FREQ_60HZ
// European power grid: 50 Hz → NOTCH_FREQ_50HZ
int humFreq = NOTCH_FREQ_60HZ;

// ── Calibration Parameters ─────────────────────────────────────────────────
// Threshold: envelope values below this are treated as baseline noise and
// zeroed out. Set during patient calibration — see header comment above.
static int Threshold = 50;

// ENVELOPE_MAX: expected peak envelope value during maximum-effort contraction.
// Used to normalize the motor PWM output across the full 0–255 range.
// Adjust per patient and electrode placement during calibration.
static int ENVELOPE_MAX = 1000;

// ── Timing ─────────────────────────────────────────────────────────────────
unsigned long timeStamp;
unsigned long timeBudget;

// ── Filter Object ──────────────────────────────────────────────────────────
EMGFilters myFilter;

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  // Initialize EMG filter: sample rate, notch frequency, bandpass on,
  // notch on, lowpass on.
  myFilter.init(sampleRate, humFreq, true, true, true);

  // Serial for calibration and debug output only.
  // Disable TIMING_DEBUG for deployment.
  Serial.begin(115200);

  // Compute per-loop time budget in microseconds.
  timeBudget = 1e6 / sampleRate;  // 1000 µs at 1000 Hz

  // Configure motor output pin.
  pinMode(OutputPin, OUTPUT);
}

// ── Main Loop ──────────────────────────────────────────────────────────────
void loop() {

  timeStamp = micros();

  // Step 1: Acquire raw EMG sample from ADC (0–1023).
  int Value = analogRead(SensorInputPin);

  // Step 2: Apply bandpass and 60 Hz notch filter.
  // Removes power line interference and out-of-band noise.
  int DataAfterFilter = myFilter.update(Value);

  // Step 3: Compute signal envelope via squaring (full-wave rectification).
  // Squaring produces a strictly positive representation of signal power.
  int envelope = sq(DataAfterFilter);

  // Step 4: Apply contraction threshold.
  // Values below threshold are treated as baseline noise and zeroed out.
  envelope = (envelope > Threshold) ? envelope : 0;

  // Step 5: Normalize envelope to 0–255 PWM range.
  // Clamp to ENVELOPE_MAX before mapping to prevent overflow and ensure
  // full motor range is achievable across all patients.
  int envelopeClamped = min(envelope, ENVELOPE_MAX);
  int PWM = map(envelopeClamped, 0, ENVELOPE_MAX, 0, 255);

  // Step 6: Output PWM to motor driver.
  // Higher muscle contraction → higher PWM → greater motor assist.
  analogWrite(OutputPin, PWM);

  // Step 7: Compute elapsed loop time for timing validation.
  timeStamp = micros() - timeStamp;

  // Debug output — disable for deployment by setting TIMING_DEBUG 0.
  if (TIMING_DEBUG) {
    Serial.print("Envelope: ");
    Serial.print(envelope);
    Serial.print("  PWM: ");
    Serial.print(PWM);
    Serial.print("  Loop µs: ");
    Serial.println(timeStamp);
  }

  // Step 8: Delay remainder of loop budget.
  // Maintains consistent 1000 Hz ADC sampling rate.
  // The integrated U-ARM system motor control loop runs at 75 Hz —
  // this higher ADC rate ensures sufficient signal fidelity for the
  // filtering and envelope detection pipeline feeding the motor controller.
  delayMicroseconds(2500);
}
