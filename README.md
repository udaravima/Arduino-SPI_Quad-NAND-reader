# Arduino SPI Quad NAND Reader

A PlatformIO project for reading SPI NAND flash memory using bit-banging on Arduino Uno (ATmega328P), with SD card dump support.

## Features

- **Quad SPI (QSPI)** bit-banging for high-speed NAND reads
- **Configurable pin mapping** for flexible hardware setups
- **SD card support** for dumping NAND contents
- **Serial command interface** for interactive control
- **Selectable NAND sizes** (128MB, 256MB, 512MB, 1GB)

## Hardware Requirements

- Arduino Uno (or compatible ATmega328P board)
- SPI NAND flash chip (8-pin package)
- SD card module with level shifter
- Jumper wires

## Pin Mapping

| Signal        | Arduino Pin | NAND Pin | SD Card |
|---------------|-------------|----------|---------|
| CLK           | D13         | Pin 6    | CLK ✓   |
| SIO0 (MOSI)   | D11         | Pin 5    | MOSI ✓  |
| SIO1 (MISO)   | D12         | Pin 2    | MISO ✓  |
| SIO2 (WP#)    | D10         | Pin 3    | -       |
| SIO3 (HOLD#)  | D9          | Pin 7    | -       |
| NAND CS#      | D8          | Pin 1    | -       |
| SD CS         | D7          | -        | CS ✓    |
| VCC           | 3.3V        | Pin 8    | VCC     |
| GND           | GND         | Pin 4    | GND     |

> **Note**: CLK, MOSI, and MISO are shared between NAND and SD card. Only one device is active at a time (controlled by CS pins).

## Wiring Diagram

```
                  Arduino Uno
                 +----------+
                 |      D13 |---> CLK (NAND Pin 6 & SD CLK)
                 |      D12 |<--> SIO1/MISO (NAND Pin 2 & SD MISO)
                 |      D11 |---> SIO0/MOSI (NAND Pin 5 & SD MOSI)
                 |      D10 |---> SIO2/WP# (NAND Pin 3)
                 |       D9 |---> SIO3/HOLD# (NAND Pin 7)
                 |       D8 |---> NAND CS# (NAND Pin 1)
                 |       D7 |---> SD CS
                 |     3.3V |---> VCC (NAND Pin 8 & SD VCC)
                 |      GND |---> GND (NAND Pin 4 & SD GND)
                 +----------+
```

## Installation

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository
3. Build and upload:
   ```bash
   pio run -t upload
   ```
4. Open serial monitor at **115200 baud**

## Serial Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `readid` | Read NAND chip ID |
| `setsize <MB>` | Set NAND size (128, 256, 512, 1024) |
| `getsize` | Show current NAND configuration |
| `readqspibytes <addr> <size>` | Read bytes using Quad SPI |
| `pageread` | Quick page read test |
| `initsd` | Initialize SD card |
| `copytosd [filename]` | Dump NAND to SD card |

## Usage Example

```
=== SPI NAND Reader ===
Chip ID: 0xEFAA

> setsize 128
NAND size set to 128MB (65536 pages)

> initsd
SD card initialized

> copytosd dump.bin
Copying 65536 pages...
Progress: 1% (64/65536)
...
Copy complete!
```

## Project Structure

```
├── src/
│   ├── config.h        # Pin definitions & NAND config
│   ├── config.cpp      # NAND size calculations
│   ├── spi_bang.h      # SPI bit-bang interface
│   ├── spi_bang.cpp    # SPI bit-bang implementation
│   ├── sd_writer.h     # SD card interface
│   ├── sd_writer.cpp   # SD card implementation
│   ├── interface.h     # Serial command interface
│   ├── interface.cpp   # Command parser
│   ├── main.h          # Main header
│   └── main.cpp        # Entry point
├── platformio.ini      # PlatformIO configuration
└── README.md
```

## Customization

### Changing Pin Assignments

Edit `src/config.h` to modify pin mappings:

```cpp
#define PIN_CLK      PB5   // D13
#define PIN_SIO0     PB3   // D11
#define PIN_SIO1     PB4   // D12
#define PIN_SIO2     PB2   // D10
#define PIN_SIO3     PB1   // D9
#define PIN_NAND_CS  PB0   // D8
#define PIN_SD_CS    7     // D7
```

### Adding NAND Size Presets

Modify `setNandSize()` in `config.cpp` for custom page/block configurations.

## Limitations

- Arduino Uno has 2KB RAM, limiting buffer size to 256 bytes
- Large NAND dumps take significant time due to serial page-by-page transfer
- Quad SPI mode only (no dual or single SPI read commands for speed)

## License

MIT License
