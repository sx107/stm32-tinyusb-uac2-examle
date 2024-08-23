# stm32-tinyusb-uac2-examle
 An example of UAC2 on stm32 using tinyusb. This project does not use the standard TinyUSB  UAC2 implementation and provides its own to remove the complex and boilerplate code that parses the descriptors. It allows you to better understand the USB audio class 2 basics and implement your own feedback mechanism.
 
 Currently only one output is supported without inputs, but this example can easily be expanded.
 
The only TinyUSB modified file is *tinyusb/device/usbd.c* to add uac2_audio.c driver.
 
It works fine with USB3300 USB PHY. The clock is setup for 24MHz crystal with 24MHz MCO1 output to USB3300 clock.

Tested on this board: https://github.com/sx107/stm32-open-audio-interface


References:
- TinyUSB: https://github.com/hathach/tinyusb
- STM32 TinyUSB intergation tutorial: https://github.com/hathach/tinyusb/discussions/633
