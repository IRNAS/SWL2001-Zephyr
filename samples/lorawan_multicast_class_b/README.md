# LoRaWAN Class B example


This sample is based on `SWSD001/apps/examples/lorawan_multicast_class_b`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

The application will automatically starts a procedure to join a LoRaWAN network.

Once a network is joined (i.e. when the corresponding event is triggered), the example will perform the requested operations to enable class B:

- Enable the time synchronization service - only `SMTC_MODEM_TIME_MAC_SYNC` is supported
- Request a ping slot update from the LNS

Once the time is updated and the ping slot update request is answered by the LNS, the example enables the class B. When the event indicating that class B is ready, the example starts a multicast session.

The example is also capable of displaying data and meta-data of a received downlink.

### Configuring TTN

See the original sample for details on how to configure TTN to send multicast downlinks.
