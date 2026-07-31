# Hardware Notes

## Power

- Arduino Uno is powered through the USB port during development.
- The RC522 RFID reader uses 3.3 V.
- All modules share a common ground.

## Functional interfaces

- The RC522 communicates over SPI.
- The 16x2 LCD communicates over I2C at address `0x27`.
- The management button is active-low and uses the Arduino internal pull-up resistor.
- LED and buzzer provide success/error feedback.

## Physical prototype

The assembled board contains the keypad, Arduino Uno, LCD, RFID reader, mode button, LED, buzzer, and supporting resistors. The firmware has been validated with this physical prototype.
