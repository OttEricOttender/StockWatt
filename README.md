# Electricity price monitoring device based on ESP-32 development board

This is a project for my bachelors paper that displays real-time electricity prices from the Nord Pool market via the [Elering API](https://dashboard.elering.ee/et/nps/price). 
It is built using an ESP32 microcontroller and a TFT LCD display (ILI9341 or ILI9488), and is designed for non-technical users — no smartphone or app required.

## Software:

I used [Arduino IDE](https://www.arduino.cc/en/software) for programming. The software utilizes Elering API which provides day-ahead prices based on the Nord Pool market. 
The device parses this data and displays the current, next 2 or 3 hour (depending on the screen), and the 24 hour maximum and minimum price. The bigger screen version also shows a 12-hour bar graph.

## Hardware used:
- ESP-32S chip
- ILI9341 or ILI9488 TFT LCD-display.
- Wires
- Breadboard

###Wiring


