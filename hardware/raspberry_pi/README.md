# Raspberry Pi Hardware Configuration
* Platform: Raspberry Pi 4 Model B (4GB RAM) or Raspberry Pi 5.
* Target OS: QNX Neutrino RTOS 7.1 / 8.0 BSP or Raspberry Pi OS (64-bit).
* Interfaces Used:
  - UART (GPIO 14 / 15): Serial NMEA GPS receiver.
  - GPIO 17, 27, 22: North-South Red, Yellow, Green relay lines.
  - GPIO 10, 9, 11: East-West Red, Yellow, Green relay lines.
  - Hardware Watchdog: `/dev/watchdog` (BCM2835 WDT).
