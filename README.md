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
IRC-A1      |CO<sub>2</sub>


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

## Notes
* I use 3 LEDs to show the CO<sub>2</sub> and PM level, whether its safe (green: PM<sub>2.5</sub> <= 32 and CO<sub>2</sub> <= 1000), warning (orange: 32 < PM<sub>2.5</sub> <= 53) and danger (red: PM<sub>2.5</sub> >53 or CO<sub>2</sub> > 1000)
* BLE module use grove universal sockets, can easily replace with your favorite module (maybe a data logger instead), just beware that the socket is connect to **5v power**. 
* ESP-01 socket is preserve for esp8266 module extension, it's fine to leave it if you don't need it.

## Reviews
1. IRC-A1 is a good sensor for CO<sub>2</sub> sensing, but with great power, you also need great skill to use it. You need to add operational amplifier (OPA) in your circuit, infact, most of my circuit and components are for the IRC-A1, include TLC271cp (OPA), IRF520 (MOSFET) and several resistor & capacitor. You also need to calibrate it, with ZERO gas (0 ppm CO<sub>2</sub>) and fix-concentration target gas (for me, 1000 ppm CO<sub>2</sub>).
I think the better circuit should use an external power with low-dropout (LDO) regulator as its power source, and use a crystal oscillator to trigger it (like it mention in its spec, but in this project, I use arduino PWM library to simulate the duty cycle it needs, maybe that's why I cannnot get the best result) .
2. The polygon of GND should not be that large, because it transfers the heat, and it will affect the DHT22 accuracy.


