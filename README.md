# RCPPMJoystick
An Arduino Leonardo/Pro Micro version of a RC PPM Trainer port to USB Joystick Converter - using Arduino Joystick Library.

Tested with [FPV Freerider](https://fpv-freerider.itch.io/fpv-freerider) (Tested on OSX, Linux, and Android using USB OTG), [Quadcopter FX Simulator Pro](https://play.google.com/store/apps/details?id=com.Creativeworld.QuadcopterFXpro&hl=en) on Android (using USB OTG Cable), and Liftoff (available via Steam) on OSX.

## Credits
- This project is based on [Leonardo-USB-RC-Adapter](https://github.com/voroshkov/Leonardo-USB-RC-Adapter), but removes the requirement to change core arduino libraries.

## Requirements:
- Single Joystick Library from https://github.com/MHeironimus/ArduinoJoystickLibrary
- an Arduino Leonardo or Arduino Pro Micro
- mono audio cable or mono audio jack and two strands of hookup wire.
- RC TX with PPM Trainer Port (Tested with Turnigy 9x)

## Circuit Diagram

Pretty simple ... 
![Circuit Diagram](https://raw.githubusercontent.com/timonorawski/RCPPMJoystick/master/CircuitDiagram.png)
