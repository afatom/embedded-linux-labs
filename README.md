# Embedded Linux Labs 🐧

A comprehensive repository for Linux Kernel development, Driver modeling, and Hardware interfacing across different ARM System-on-Chips (SoCs).

## Objectives
- Master **Kernel Space** programming (Timers, IRQs, Atomic Context).
- Explore **Bus Protocols** (I2C, SPI, UART) at the driver level.
- Implement **Sysfs & IOCTL** interfaces for user-space communication.
- Compare implementation details between **Broadcom (RPi)** and **TI Sitara (BBB)** architectures.

## Supported Platforms
| Platform | SoC | Architecture |
| :--- | :--- | :--- |
| **Raspberry Pi 4B** | BCM2711 | ARMv8 (Cortex-A72) |
| **BeagleBone Black** | AM335x | ARMv7 (Cortex-A8) |
| **QEMU** | Virt | ARMv7/v8 Emulation |

## Repository Roadmap

### Raspberry Pi 4B
- [x] **01-timer-sysfs**: Periodic timer with start/stop control via Sysfs.
- [ ] **02-gpio-interrupts**: Handling hardware debouncing and IRQ top/bottom halves.

### BeagleBone Black
- [ ] **01-pru-messaging**: Interfacing with the Programmable Real-time Unit.
- [ ] **02-device-tree-overlays**: Custom overlays for external sensors.

## Cross-Compilation Guide
To build for the Raspberry Pi from a Linux/WSL host:
```bash
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
make -C /path/to/pi/linux/headers M=$(pwd) modules
