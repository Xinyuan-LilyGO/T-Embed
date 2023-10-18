# Example of how to use the T-Embed with ESP-IDF V5.0

## Supported Peripherals

This example shows:

1. LCD with LVGL
2. Dial knob with push button (shows up in monitor output)
3. LED strip around the dial

## Prerequisites

1. A T-Embed (preferably working)
2. An installation of the ESP-IDF V5.0 including ESP32S3 target
3. This repository
4. A working USB data and power cable

## Getting Started

0. Plug in your T-Embed. Make sure it is recognized by the OS (I recommend linux)
1. Clone the ESP-IDF repository V5.0 branch
   `git clone --depth=1 --recursive -b v5.0 https://github.com/espressif/esp-idf.git`
2. Run the `install.sh` in the ESP-IDF repo root `install.sh all`
3. Export the ESP-IDF config into the shell `. ./export.sh`
4. Check the IDF version `idf.py --version` (should be v5.0)
5. Clone this repo
6. Change to the `example/esp-idf-v5.0` dir
7. `idf.py flash && idf.py monitor`

## Expected output

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x15 (USB_UART_CHIP_RESET),boot:0xb (SPI_FAST_FLASH_BOOT)
Saved PC:0x40048d17
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3810,len:0x17d8
load:0x403c9700,len:0xe88
load:0x403cc700,len:0x3000
SHA-256 comparison failed:
Calculated: 3c4dc30344ef58d708ed5cb5d936eee67c311244cdd3e6b9576b61d747836d66
Expected: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
Attempting to boot anyway...
entry 0x403c9930
I (43) boot: ESP-IDF v5.0 2nd stage bootloader
I (43) boot: compile time 14:09:47
I (43) boot: chip revision: v0.1
I (45) boot_comm: chip revision: 1, min. bootloader chip revision: 0
I (52) qio_mode: Enabling default flash chip QIO
I (57) boot.esp32s3: Boot SPI Speed : 80MHz
I (62) boot.esp32s3: SPI Mode       : QIO
I (66) boot.esp32s3: SPI Flash Size : 16MB
I (71) boot: Enabling RNG early entropy source...
I (77) boot: Partition Table:
I (80) boot: ## Label            Usage          Type ST Offset   Length
I (88) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (95) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (103) boot:  2 factory          factory app      00 00 00010000 00177000
I (110) boot: End of partition table
I (114) boot_comm: chip revision: 1, min. application chip revision: 0
I (122) esp_image: segment 0: paddr=00010020 vaddr=3c060020 size=2da1ch (186908) map
I (149) esp_image: segment 1: paddr=0003da44 vaddr=3fc94900 size=025d4h (  9684) load
I (151) esp_image: segment 2: paddr=00040020 vaddr=42000020 size=521ach (336300) map
I (189) esp_image: segment 3: paddr=000921d4 vaddr=3fc96ed4 size=00f74h (  3956) load
I (190) esp_image: segment 4: paddr=00093150 vaddr=40374000 size=108e8h ( 67816) load
I (205) esp_image: segment 5: paddr=000a3a40 vaddr=50000000 size=00010h (    16) load
I (212) boot: Loaded app from partition at offset 0x10000
I (212) boot: Disabling RNG early entropy source...
I (227) octal_psram: ECC is enabled
I (227) octal_psram: vendor id    : 0x0d (AP)
I (227) octal_psram: dev id       : 0x02 (generation 3)
I (231) octal_psram: density      : 0x03 (64 Mbit)
I (236) octal_psram: good-die     : 0x01 (Pass)
I (241) octal_psram: Latency      : 0x01 (Fixed)
I (247) octal_psram: VCC          : 0x01 (3V)
I (252) octal_psram: SRF          : 0x01 (Fast Refresh)
I (257) octal_psram: BurstType    : 0x00 ( Wrap)
I (263) octal_psram: BurstLen     : 0x03 (1024 Byte)
I (268) octal_psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (275) octal_psram: DriveStrength: 0x00 (1/1)
W (280) PSRAM: DO NOT USE FOR MASS PRODUCTION! Timing parameters will be updated in future IDF version.
I (290) esp_psram: Found 8MB PSRAM device
I (294) esp_psram: Speed: 80MHz
I (330) mmu_psram: Instructions copied and mapped to SPIRAM
I (345) mmu_psram: Read only data copied and mapped to SPIRAM
I (345) cpu_start: Pro cpu up.
I (345) cpu_start: Starting app cpu, entry point is 0x40375678
0x40375678: call_start_cpu1 at cpu_start.c:142

I (0) cpu_start: App cpu up.
I (737) esp_psram: SPI SRAM memory test OK
I (746) cpu_start: Pro cpu start user code
I (746) cpu_start: cpu freq: 240000000 Hz
I (746) cpu_start: Application information:
I (749) cpu_start: Project name:     esp_idf_v5.0_tembed
I (755) cpu_start: App version:      654a010-dirty
I (761) cpu_start: Compile time:     Jan 16 2023 15:53:20
I (767) cpu_start: ELF file SHA256:  4f84fc36fdb69bfc...
I (773) cpu_start: ESP-IDF:          v5.0
I (777) heap_init: Initializing. RAM available for dynamic allocation:
I (785) heap_init: At 3FCA0F60 len 000487B0 (289 KiB): D/IRAM
I (791) heap_init: At 3FCE9710 len 00005724 (21 KiB): STACK/DRAM
I (798) heap_init: At 3FCF0000 len 00008000 (32 KiB): DRAM
I (804) heap_init: At 600FE010 len 00001FF0 (7 KiB): RTCRAM
I (810) esp_psram: Adding pool of 7104K of PSRAM memory to heap allocator
I (818) spi_flash: detected chip: gd
I (822) spi_flash: flash io: qio
I (826) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (846) esp_psram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (846) hello_lcd: Hello lcd!
This is esp32s3 chip with 2 CPU core(s), WiFi/BLE, silicon revision v0.1, 16MB external flash
Minimum free heap size: 7570560 bytes
I (866) gpio: GPIO[46]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (876) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (886) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (896) lcd: Backlight off
I (896) gpio: GPIO[15]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (906) lcd: Init SPI bus
I (906) lcd: Init panel io
I (916) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (926) lcd: Attach panel
I (926) gpio: GPIO[9]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (936) lcd: Init lcd
I (2056) lcd: Enable lcd
I (2056) lcd: Backlight on
I (2056) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (2056) gpio: GPIO[2]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3
I (2066) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3
I (2076) Knob: Iot Knob Config Succeed, encoder A:2, encoder B:1, direction:0, Version: 0.1.0
I (2086) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (2096) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (2106) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (2116) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (2126) lvgl: Init
I (2126) lvgl: Register display
I (2126) lvgl: Tick timer
I (2136) hello_lcd: Display LVGL

```

You should see three labels on the screen "RED", "GREEN", "BLUE" in the correct colors

If you turn the dial the number on the bottom of the screen should change
