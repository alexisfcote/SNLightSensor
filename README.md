# SNLightSensor
Author: Alexis Fortin-Côté (2019)

Arduino project using Adafruit Adalogger 32u4, adafruit RTC featherwing and Sparkfun Spectral Sensor Breakout to log sensor to SD card with accurate timestamp.

The shipped libraries need to be installed in the Arduino IDE to compile.

The LightSensor folder contains the program for the Microcontroller.

The microcontroller makes a reading of the sensor, time from the RTC and battery voltage every 8sec and writes to the sd card after 8 readings to save on battery. The microcontroller put itself into deep spleep between readings.

The RTC should be accurate to ±5sec per year. The coin battery used for back up power to the RTC should last for 7 years.

The 500mAh LiPo battery can last 1 day and 16 hours. The 2000mAh LiPo battery should therefore last 6 days and 18 hours.

1 week of recordings amounts to about 1MB of data. The 32GB sd card should therefore provide virtually limitless record space.

On power on the board checks if everything is in place. If something is missing (RTC, SD card or Spectral sensor) it rapidly blink indefinitely. Check the connection and restart the board by removing LiPo battery and usb power. The borad will send initialization messages on the serial port (9600 baud) at boot. This can be used for diagnostic.

By programming the board with the AS7262 program (in the AS7262 folder), the python_monitor.py app can be used to make continuous reading and display of the sensor.

The board is made of those parts:
Adafruit Feather 32u4 Adalogger
DS3231 Precision RTC Featherwing
Sparkfun Qwiic Adapter (soldered on the RTC Featherwing)
Sparkfun Spectral Sensor Borad AS7262 Visible