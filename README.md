# RFID-Based Smart Door Access Control System

An Arduino Uno-based RFID access-control prototype for a smart door lock. The embedded system authenticates RFID cards, supports keypad-based administrative actions, and persists authorized card data in EEPROM.

> **Repository scope:** this repository contains the verified embedded firmware and hardware documentation for the access-control system.

## Highlights

- RFID authentication using the MFRC522 / RC522 reader
- One master card and up to ten guest cards stored persistently in Arduino EEPROM
- 4x4 keypad for administrator and unlock-password entry
- LCD status feedback, LED indication, and buzzer alerts
- Card enrollment, guest-card removal, and bulk guest-card deletion
- Failed-access protection after five invalid-card attempts
- Functional prototype implemented and validated on physical hardware

## Hardware

| Component | Purpose |
| --- | --- |
| Arduino Uno | Main controller |
| RC522 (MFRC522) | RFID card reader |
| 4x4 matrix keypad | Password and administrative input |
| 16x2 I2C LCD | System status display |
| Buzzer | Audible feedback |
| LED + resistor | Visual status feedback |
| Push button | Cycles administrative modes after admin authentication |

### Pin mapping

| Arduino pin | Connected module / function |
| --- | --- |
| D2-D5 | Keypad rows R1-R4 |
| D6-D9 | Keypad columns C1-C4 |
| D10 | RC522 SDA / SS |
| D11 | RC522 MOSI |
| D12 | RC522 MISO |
| D13 | RC522 SCK |
| A3 | RC522 RST |
| A4 | LCD SDA |
| A5 | LCD SCL |
| A0 | Buzzer |
| A1 | Mode button (`INPUT_PULLUP`) |
| A2 | Status LED |

> The RC522 must be powered from **3.3 V**. Do not connect its VCC pin to 5 V.

## Firmware setup

1. Install Arduino IDE and select **Arduino Uno**.
2. Install these libraries using Library Manager:
   - `MFRC522` by GithubCommunity
   - `LiquidCrystal I2C`
   - `Keypad`
3. Open [firmware/rfid_smart_door_lock.ino](firmware/rfid_smart_door_lock.ino).
4. Review and replace the demo passwords before flashing:
   - `MASTER_PASSWORD`: authorizes administrative modes.
   - `UNLOCK_PASSWORD`: unlocks access after the failed-card limit.
5. Upload the sketch to the board.

## Operating flow

1. Present an authorized master or guest RFID card to grant access.
2. Press `A`, enter the administrator password, then press `#` to authenticate for management.
3. Press the hardware button to cycle through modes:
   - Add master card
   - Add guest card
   - Delete a card
   - Delete all guest cards (requires presenting the master card)
4. Present a card when prompted on the LCD.

Press `*` while entering a password to delete the last character; press `#` to submit.

## Persistence and security notes

Authorized-card UIDs are saved in EEPROM and remain available after a power cycle. The included passwords are demonstration values and must be changed before real deployment. This project is an educational prototype; production deployments should use secure credential storage, a stronger access-control policy, tamper protection, and non-blocking timing logic.

## Demonstration assets

The project includes a Proteus schematic and a physical working prototype. Add the original files below before publishing if available locally:

- `docs/images/system-schematic.png`
- `docs/images/working-prototype.jpg`

## Project structure

```text
.
├── firmware/       # Arduino sketch
├── docs/           # Hardware and project documentation
├── README.md       # Project overview and setup guide
├── LICENSE
└── .gitignore
```

## License

Distributed under the [MIT License](LICENSE).
