# Zephyr OS

This is a fork of the Zephyr Kernel v1.0.0, an embedded operating system
supporting the Arduino 101. See the project site at
https://www.zephyrproject.org/downloads/zephyr-kernel.

## Prerequisites

Install the SDK:

```bash
sudo apt-get install git make gcc gcc-multilib g++ libc6-dev-i386 \
    g++-multilib

wget https://nexus.zephyrproject.org/content/repositories/releases/org/zephyrproject/zephyr-sdk/0.7.2-i686/z

chmod +x zephyr-sdk-0.7.2-i686-setup.run
sudo ./zephyr-sdk-0.7.2-i686-setup.run

# Put these in /etc/environment
ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk 
ZEPHYR_GCC_VARIANT=zephyr
```

## Build and Flash

Information to build and flash is provided at
https://www.zephyrproject.org/doc/board/arduino_101.html .

One goal of this forked codebase is to drastically simplify the
development flow by offering a complete one-pass build system. For that,
do:

```
cd zephyr-kernel
. zephyr-env.sh
cd build/arduino_101/

make image
make flash

```
