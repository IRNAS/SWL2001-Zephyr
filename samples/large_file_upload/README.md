# Lorawan sample

This sample is based on `SWSD001/apps/examples/large_file_upload`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

The device will attempt to join the LoRaWAN network.
After joining, an uplink message will be sent every `APP_TX_DUTYCYCLE`.
Downlinks can also be received.
