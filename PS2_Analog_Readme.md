# Reflex Adapt - PS2 Analog Button Support

**DualShock 2 pressure-sensitive button firmware for Reflex-Adapt**

This firmware adds native analog button support for the Sony DualShock 2 controller, enabling pressure-sensitive functionality for Circle, Square, L2, and R2 buttons on via PCSX2 emulation. Rumble is unfortunately not supported due to limitations of D Input, while X Input (which _does_ support rumble) does not support analog face buttons. Go figure!

This firmware was developed specifically for MGS3; it also works well for MGS2. If there's demand I could make another firmware optimized for other games like GTA and Gran Turismo. See below for the technical reasons for this approach.

I have personally tested this firmware using PCSX2 on Windows and Linux; macOS support is implied but not guaranteed. If there's ever a "MiSTer 2" with a PS2 core perhaps it will work there too.

## Features

- **Pressure-Sensitive Buttons**: Circle, Square, L2, R2 with full 0-255 analog range; analog sticks work as expected
- **Digital Button Preservation**: Triangle, Cross, L1, R1, D-pad, Start, Select, L3, R3 remain digital
- **PCSX2 Compatible**: Proper pressure value inversion for PCSX2
- **Game-Optimized**: Optimized for MGS3 (and MGS2), see below.
- **OLED Display**: Clear "PS2 ANALOG" firmware identification, buttons light up when pressed

## Technical Implementation

### HID Axis Allocation

The DualShock 2 reports all face buttons and shoulders as pressure-sensitive (0–255). However, USB HID gamepads on Windows are effectively limited to 8 analog axes (X, Y, Z, Rx, Ry, Rz, Slider1, Slider2). 

After mapping both analog sticks (4 axes), only 4 remain. These are strategically assigned for MGS3:

**Analog Buttons:**
- **Square** - Gun control
- **Circle** - CQC  
- **L2** - Item menu feathering
- **R2** - Weapon menu feathering

**Digital Buttons:**
- All other buttons (D-pad, △, ×, L1, R1, L3, R3, Start, Select) mapped as digital

This design preserves critical analog behavior while staying within HID axis limits.

## Installation

1. Use the [Reflex-Adapt updater tool](https://github.com/misteraddons/Reflex-Adapt) to flash the MGS23.hex firmware
2. Connect your DualShock 2 controller to the Reflex-Adapt
3. The OLED display will show "PS2 ANALOG" to confirm the specialized firmware is loaded

## Building from Source

1. Follow the [main Reflex-Adapt build instructions](https://github.com/misteraddons/Reflex-Adapt#building-firmware-on-linux)
2. Use the modified source files in this repository
3. Build with: `make MGS23`
4. The compiled firmware will be available as `firmware/MGS23.hex`

## Technical Details

- **Memory Usage**: 68% flash (19,506 bytes), 53% RAM (1,361 bytes)
- **Pressure Resolution**: Full 8-bit (0-255) analog range
- **PCSX2 Compatibility**: Automatic pressure value inversion (255 - value)
- **Specialized Architecture**: PS2-only firmware for optimal performance

## Files Modified

- `ReflexMPG/Input_Psx.h` - DualShock 2 input handling and analog button enablement
- `ReflexMPG/src/MPG/MPG.cpp` - HID report generation and button mapping
- `ReflexMPG/ReflexMPG.ino` - OLED display enhancement
- `manifest.txt` - Firmware distribution configuration
- `Makefile` - Added MGS23 build target for Metal Gear Solid optimized firmware

## License

Same as main Reflex-Adapt project.

## Author

Created by Nicholas De Leon // github.com/daylayown // nicholas@daylayown.org // @nicholasadeleon (X and Bluesky)

## Credits

Reflex-Adapt by MisterAddons (github.com/misteraddons)
PsxNewLib by SukkoPera (github.com/SukkoPera)
