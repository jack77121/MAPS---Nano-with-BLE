# MAPS-Arduino Nano
![Image of PCB layout]
(https://github.com/jack77121/MAPS---Nano-with-BLE/blob/master/MAPS_BLE_appearance.png)

## Story
**M**icro **A**ir **P**ollution **S**ensing - MAPS

Air pollution is a hot topic in recent years, especially - **Particulate Matter**. Traditional sensing method, we can only depend on professional sensors/devices, with large volume, heave weight and insanity price.
Thanks to the technology, now we could build our own air pollution sensing station, with micro volume, light weight and the price is affordable :). With MAPS, you can sense air pollution any where, any time by yourself, you can also give it a battery module to increase its mobility!

For more air pollution sensing project, you can visit [LASS facebook](https://www.facebook.com/groups/1607718702812067/) and [LASS github](https://github.com/LinkItONEDevGroup/LASS), we are makers who sense anything in our environment!

## Hardware & Sensors
Module      |	Description
------------|--------------
Arduino Nano|Mainboard
RTC         |time sync
BLE         |pair with smart phone
G3 (pms3003)|Particalute matter (PM)
DHT22       |Temperature & humidity
BMP180      |Ambient pressure
IRC-A1      |CO_2


## PCB layout & dimensions
![Image of pcb layout]
(https://github.com/jack77121/MAPS---Nano-with-BLE/blob/master/PCB_layout.png)

(unit: mm)

## Installation
1. Upload *MAPS_BLE.ino* to your nano.
2. Compile and install *MAPS_ble* to android phone.
3. Choose to pair the MAPS BLE module from app.
4. You are ready to see the sensors data on you phone.

![Image of app after pairing]
(https://github.com/jack77121/MAPS---Nano-with-BLE/blob/master/app_figure.png)

## Reviews
1. IRC-A1 is a good sensor for CO_2 sensing, but with great power, you also need great skill to use it. You need to add operational amplifier (OPA) in your circuit, infact, most of my circuit and components are for the IRC-A1, include TLC271cp (OPA), IRF520 (MOSFET) and several resistor & capacitor. You also need to calibrate it, with ZERO gas (0 ppm CO_2) and fix-concentration target gas (for me, 1000 ppm CO_2).
2. 




(Continue updating...)

