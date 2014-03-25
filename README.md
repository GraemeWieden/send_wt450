WT450 Emulator
==============

WT450 Temperature and Humidity emulator for Arduino
---------------------------------------------------

This utility allows an Arduino to emulate the standard Ninja Blocks temperature and humidity sensor.

By default the transmitter data pin should be connected to the Arduino digital pin 9.

Use the sendWRT450Packet to send a custom packet specifying:

- House Code (1 to 15)
- Channel (1 to 4)
- Humidity (0 to 127)
- Temperature (floating point 0 to 205)

Acknowledgements
----------------
Thank you to Jaakko Ala-Paavola for the work he and his colleagues did in decoding and publishing the WT450 protocol and also for allowing me to include the details in this utility.
