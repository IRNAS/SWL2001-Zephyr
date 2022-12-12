# Lorawan asynchronous sample

This sample is based on `SWSD001/apps/examples/lorawan_asynchronous`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

The sample first initializes the Basics modem and button 2 on the DK.
The device will attempt to join the LoRaWAN network.
After joining, a press on `button 2` on the DK will send an uplink message with the number of button presses.
