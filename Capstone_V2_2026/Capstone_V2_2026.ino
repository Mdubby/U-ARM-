#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "EMGFilters.h"

#define TIMING_DEBUG 1

#define SensorInputPin A0 // input pin number
#define OutputPin 7      // output pin number

EMGFilters myFilter;
// discrete filters must work with a fixed sample frequency
// our emg filter only supports "SAMPLE_FREQ_500HZ" or "SAMPLE_FREQ_1000HZ"
// other sampleRate inputs will bypass all the EMG_FILTER
int sampleRate = SAMPLE_FREQ_1000HZ;
// For countries where power transmission is at 50 Hz
// For countries where power transmission is at 60 Hz, need to change to
// "NOTCH_FREQ_60HZ"
// our emg filter only supports 50Hz and 60Hz input
// other inputs will bypass all the EMG_FILTER
int humFreq = NOTCH_FREQ_50HZ;

// Calibration:
// put on the sensors, and release your muscles;
// wait a few seconds, and select the max value as the threshold;
// any value under the threshold will be set to zero
static int Threshold = 50;

unsigned long timeStamp;
unsigned long timeBudget;

void setup() {
  /* add setup code here */
  myFilter.init(sampleRate, humFreq, true, true, true);

  // open serial
  Serial.begin(115200);
  // setup for time cost measure
  // using micros()
  timeBudget = 1e6 / sampleRate;
  // micros will overflow and auto return to zero every 70 minutes

  pinMode(OutputPin, OUTPUT); // Set digital pin 7 as an output
}

void loop() {
  /* add main program code here */
  // In order to make sure the ADC sample frequency on Arduino,
  // the time cost should be measured each loop
  /*------------start here-------------------*/
  timeStamp = micros();

  int Value = analogRead(SensorInputPin);

  // filter processing
  int DataAfterFilter = myFilter.update(Value);

  int envelope = sq(DataAfterFilter);
  // Normalize and clamp to PWM range
int envelopeClamped = min(envelope, 1000);
int PWM = map(envelopeClamped, 0, 1000, 0, 255);

  // Output PWM value to pin 7
  analogWrite(OutputPin, PWM);

  timeStamp = micros() - timeStamp;
  if (TIMING_DEBUG) {
    // Serial.print("Read Data: "); Serial.println(Value);
    // Serial.print("Filtered Data: ");Serial.println(DataAfterFilter);
    Serial.print("Squared Data: ");
    Serial.println(envelope);
    // Serial.println(PWM);

    // Serial.print("Filters cost time: "); Serial.println(timeStamp);
    // the filter cost average around 520 us
  }

  /*------------end here---------------------*/
  // if less than timeBudget, then you still have (timeBudget - timeStamp) to
  // do your work
  delayMicroseconds(2500);
  // if more than timeBudget, the sample rate needs to be reduced to
  // SAMPLE_FREQ_500HZ
}
