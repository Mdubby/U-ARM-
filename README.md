# U-ARM — Upper Arm Rehabilitation Machine
### BME 491: Bioengineering Design and Analysis | University of Nevada, Reno | Spring 2023

The U-ARM is an EMG-driven rehabilitation exoskeleton designed for patients with neurological impairments who lack voluntary arm function. An EMG sensor detects bicep muscle activity, which is processed by firmware running on an ATmega2560 microcontroller and converted into a proportional PWM signal that drives a gimbal motor — assisting the patient through an arm-curling rehabilitation motion.

---

## How It Works

### System Overview

![uarm_patient_schematic](https://github.com/user-attachments/assets/b622e941-a7b6-4df0-8bbc-135e0f14479f)

![uarm_signal_pipeline](https://github.com/user-attachments/assets/587f099f-4dbc-4315-a66d-292929cb6de3)

---

### Signal Pipeline

| Stage | Operation | Code | What it removes |
|-------|-----------|------|-----------------|
| 1. Acquisition | Read raw EMG from electrode | `analogRead(SensorInputPin)` | — |
| 2. Filtering | Bandpass + 60 Hz notch | `myFilter.update(Value)` | Power line interference, motion artifact |
| 3. Envelope | Square the filtered signal | `sq(DataAfterFilter)` | Signal polarity (converts to positive power) |
| 4. Threshold | Gate below noise floor | `envelope > Threshold ? envelope : 0` | Baseline muscle noise |
| 5. PWM output | Map envelope to motor speed | `analogWrite(OutputPin, pwm)` | — |

### Key Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Sample rate | 1000 Hz | `SAMPLE_FREQ_1000HZ` |
| Notch frequency | 60 Hz | `NOTCH_FREQ_60HZ` — US power grid |
| Contraction threshold | ~50 (tunable) | Set during patient calibration |
| Max envelope | ~1000 (tunable) | Adjust per patient signal strength |
| PWM range | 0–255 | Maps directly to motor duty cycle |
| Loop timing | 2500 µs delay | ~400 Hz effective control loop |
| Output pin | Digital 7 | PWM-capable on ATmega2560 |
| Input pin | Analog A0 | EMG sensor signal input |

---

## Hardware Components

| Component | Function |
|-----------|----------|
| ATmega2560 (Arduino Mega) | Main microcontroller |
| EMG sensor (MyoWare or equivalent) | Surface EMG acquisition from bicep |
| Gimbal motor | Provides assisted arm-curl motion |
| Power shield / H-bridge | Motor drive circuit |
| 3D-printed PLA exoskeleton brace | Physical arm support structure |
| Electrode pads | Skin-contact EMG pickup |

---

## Firmware Overview

### `setup()`

Initializes the EMG filter library at 1000 Hz with a 60 Hz notch, configures output pin 7 as PWM, opens serial at 115200 baud for debugging, and computes the per-loop time budget from sample rate.

### `loop()`

Runs continuously at approximately 400 Hz. Each iteration:

1. Records timestamp with `micros()`
2. Reads raw EMG value from pin A0
3. Passes raw value through the EMGFilters bandpass/notch filter
4. Squares the filtered output to compute signal envelope
5. Zeroes out values below the calibration threshold
6. Clamps envelope to max expected range and maps to 0–255 PWM
7. Writes PWM to pin 7 (motor output)
8. Prints squared envelope to serial for monitoring
9. Delays 2500 µs to maintain consistent loop timing

### Calibration

Before each session, place the electrodes on the patient's bicep and rest the arm fully relaxed for several seconds. Observe the peak serial output value during rest — set `Threshold` to approximately 20% above this resting value to suppress baseline noise. Perform a maximum-effort contraction and observe the peak envelope value — set this as the maximum for the `map()` function to ensure full motor range across the patient's contraction range.

---

## Libraries Required

```cpp
#include "EMGFilters.h"      // Grove EMG Filter library
#include "Arduino.h"         // Arduino core
```

Install `EMGFilters` via the Arduino Library Manager or from the [Seeed Studio EMGFilters repository](https://github.com/Seeed-Studio/EMGFilters).

---


## Project Documentation

Full business plan, system schematic, Gantt chart, and hardware photos are available in the `UARM_Buisness_Plan.dox` folder.

---

## Authors

**Matthew Wallace** — Biomedical Engineering, University of Nevada, Reno (B.S. 2023)
linkedin.com/in/mattwallace-bme

**Monte Howell** — Biomedical Engineering, University of Nevada, Reno (B.S. 2023)

**Jose Moreno Duran** — Biomedical & Electrical Engineering, University of Nevada, Reno (B.S. 2023)

**Kirsche Stanton** — Biomedical Engineering & Accounting, University of Nevada, Reno (B.S. 2023)
   



---

## Related Files

- `Capstone_V2.ino` — EMG acquisition, filtering, and motor control firmware
- `docs/CPE301_Final_Project_Overview.pdf` — Swamp cooler embedded FSM (related embedded systems work)
- `docs/UARM_Business_Plan_Professional.docx` — Full business plan and market analysis
- `docs/swamp_cooler_state_diagram.svg` — FSM state diagram reference
