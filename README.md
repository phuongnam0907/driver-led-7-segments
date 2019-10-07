# driver-led-7-segments
Using GPIO, device tree overlay.

[DEMO on Youtube](https://youtu.be/ipFCuwkLi50)


Hardware details: 
1. [LED 7 SEGMENT](https://hshop.vn/products/mach-hien-thi-8-led-7-doan)
2. Board Raspberry Pi 3

## Add device tree overlay

1. Enter the linux source, mostly is: <i>/{linux-folder}/arch/arm/boot/dts/overlays/</i>

      In my case: <i>"/home/lephuongnam/rpi3/outsource/linux/arch/arm/boot/dts/overlays"</i>

2. Add file <b>gpio-led-seg-overlay.dts</b>, then add these lines:
* <i>gpio-led-seg.dtbo</i> in <i>Makefile</i> file.
* <i>RPI_KERNEL_DEVICETREE_OVERLAYS += "overlays/gpio-led-seg.dtbo"</i> in file machine config.
* <i>RPI_EXTRA_CONFIG = "dtoverlay=gpio-led-seg"</i> in <i>local.conf</i> file.

## Build environment

Using environment of Poky building for Raspberry Linux Kernel, and linux header 4.15.

Run environment

```
source /opt/poky/2.7/environment-setup-armv7vet2hf-neon-poky-linux-gnueabi
```

## How to run

Make file

```
make all
```

<i>led7gpio.ko</i> file will be created, then copy it to raspberry.

Then run follow these:
1. Install module
```
insmod led7gpio.ko 
```
2. Run module
```
echo 12345678 > /sys/class/led7control/led7controls/setled
```
NOTE: input is a positive number with length less more than 9 (from 0 to 99999999). If input has length longer, LED will be shown eight first characters.

You can stop/reset this LED
```
echo stop > /sys/class/led7control/led7controls/setled
```
3. Remove module
```
rmmod led7gpio.ko
```

## Run in Example

```
insmod led7gpio.ko 
echo 12345678 > /sys/class/led7control/led7controls/setled
echo 12331231231278 > /sys/class/led7control/led7controls/setled
echo 1039813 > /sys/class/led7control/led7controls/setled
echo stop > /sys/class/led7control/led7controls/setled
rmmod led7gpio.ko

```

