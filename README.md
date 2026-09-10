# Smart Home

A 3D-printed model house with a working alarm system inside it — motion, door contacts, climate and lighting, split across two microcontrollers.

Bachelor's thesis, Faculty of Robotics, Politehnica University of Timisoara, 2022.
Design, electronics and firmware: **Eng. Narcis Pantea**.

**📄 Project page → https://naarcis-git.github.io/smart-home/**

---

## What it does

| | |
|---|---|
| **Arms and disarms with a code** | 4×4 membrane keypad on the front wall. Press `*`, type the four-digit code, confirm with `#`. |
| **Sees movement inside** | PIR sensor. Motion while armed fires the buzzer, the alarm LED and a message on the display. |
| **Knows if a door is open** | Magnetic reed contact. The system refuses to finish arming until the door is actually shut. |
| **Reports temperature and humidity** | DHT11, sampled once a second, pushed over Wi-Fi to a phone dashboard with live gauges and history. |
| **Turns the lights on by itself** | Photoresistor. Below a set threshold the indoor LEDs switch on with no input. |

## Architecture

Two boards, deliberately separated:

- **Arduino Uno** owns security — keypad, PIR, reed contact, buzzer, LCD, alarm LED. It never depends on the network. Cutting the Wi-Fi does not disable the alarm; it only stops the reporting.
- **ESP32 Wemos Lolin32** owns everything that leaves the house — DHT11, photoresistor, indoor LEDs, and the Blynk uplink.

### State machine (Uno)

```
DISARMED  --- press *, code --->  IN_PROGRESS  --- door closed --->  ARMED
    ^                                  |                               |
    |                                  '-- door still open: "Error"    |
    '------------------- code entered, stand down ---------------------'

ARMED + motion, or ARMED + door opened  ->  alarm (buzzer + LED + display)
```

Three states rather than two. An alarm that only knows *on* and *off* will happily
arm itself around an open door; this one waits in the middle state until the
perimeter is genuinely closed.

### Telemetry (ESP32)

| Virtual pin | Carries |
|---|---|
| `V4` | Temperature, °C |
| `V5` | Relative humidity, % |
| `V0` | Light reading that drives the lamps |

A `BlynkTimer` calls `sendSensor()` once a second. Widgets on the dashboard bind
to datastreams rather than to pins, so the display can be relabelled and rescaled
without reflashing the board.

## Hardware

| Part | Role | Board |
|---|---|---|
| Keypad 4×4 (membrane) | Code entry to arm and disarm | Uno |
| PIR HC-SR501 | Movement inside the room | Uno |
| Reed contact + relay module | Whether the door is shut | Uno |
| Alarm LED | Visual alert | Uno |
| Piezo buzzer | Audible alert | Uno |
| LCD 1602A over I²C (`0x27`) | Status text, 16×2 characters | Uno |
| DHT11 | Temperature and relative humidity | ESP32 |
| Photoresistor | Ambient light, read as analogue | ESP32 |
| Indoor LEDs ×4 | Automatic lighting | ESP32 |

The shell was drawn in ArchiCAD and printed in sections on a Prusa i3 and an
Ender 5 Pro — 1.75 mm filament in, 0.04 mm layers out, 200 × 200 × 200 mm build
volume — then assembled under a clear acrylic cover.

## Repository layout

```
.
├── index.html                          project page (GitHub Pages)
├── firmware/
│   ├── uno_security/
│   │   ├── uno_security.ino            keypad, PIR, reed, buzzer, LCD, state machine
│   │   └── config.example.h            → copy to config.h
│   └── esp32_telemetry/
│       ├── esp32_telemetry.ino         DHT11, photoresistor, LEDs, Blynk uplink
│       └── secrets.example.h           → copy to secrets.h
├── .gitignore
└── LICENSE
```

## Building

Arduino IDE, one sketch per board.

**Uno** — libraries: `LiquidCrystal_I2C`, `Keypad`.

```
cp firmware/uno_security/config.example.h firmware/uno_security/config.h
```

Then set `ACCESS_CODE` in `config.h` and upload.

**ESP32** — install the ESP32 board package, plus the `Blynk` and `DHT sensor library` libraries.

```
cp firmware/esp32_telemetry/secrets.example.h firmware/esp32_telemetry/secrets.h
```

Then fill in your Blynk template ID, device name and auth token, and your Wi-Fi
credentials, and upload.

Neither `config.h` nor `secrets.h` is committed — both are in `.gitignore`.

## A note on this code

This is the 2022 firmware, published close to how it was submitted. Three things
were changed on the way into this repository, and nothing else:

1. **Credentials were taken out of the source.** The Wi-Fi password, the Blynk
   auth token and the keypad access code were compiled straight into the
   sketches. They now live in ignored header files, with committed examples.
2. **A duplicated blocking call was removed.** `sendSensor()` was being called
   both from the one-second timer and directly from `loop()`, and contained its
   own `delay(1000)`. The timer alone now drives it.
3. **Serial messages and comments were translated to English.**

What I would change if I built it today — a local MQTT broker instead of a
vendor cloud, motion corroborated against the door contact and a light baseline
rather than trusted alone, a learned threshold instead of a hard-coded `200`,
and a timestamped log so a false alarm at three in the morning can be explained
the next day — is written up on the project page.

## Licence

MIT — see [LICENSE](LICENSE).
