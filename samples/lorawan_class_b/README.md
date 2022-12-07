# LoRaWAN Class B example


This sample is based on `SWSD001/apps/examples/lorawan_class_b`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

**This sample requires a class B enabled gateway to work**
The application will automatically start a procedure to join a LoRaWAN network.
Once a network is joined (i.e. when the corresponding event is triggered), the example will perform the required operations to enable class B.
The application is also capable of displaying data and meta-data of a received downlink. Since this is class B, the downlink can be received without sending any uplinks.
