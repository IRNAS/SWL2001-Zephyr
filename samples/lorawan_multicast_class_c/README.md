# Lorawan Multicast Class C

This sample is based on `SWSD001/apps/examples/lorawan_multicast_class_c`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

The application will automatically starts a procedure to join a LoRaWAN network.

Once a network is joined (i.e. when the corresponding event is triggered), a multicast group is configured, and a multicast session using that multicast group is started. At this point, the application is ready to receive multicast downlinks. Otherwise, this application is similar to `lorawan` example, and the application periodically requests uplinks each time the LoRa Basics Modem alarm event is triggered.

The content of the uplink is the value read out from the charge counter by calling `smtc_modem_get_charge()`.

The application displays data and meta-data of standard and multicast downlinks.

if `MULTICAST_ENABLED` is set to `false`, the sample will function as a normal class C device - downlinks can be received at any point.
If set to `true`, only multicast downlinks can be received by the device (TODO: check if this is expected behaviour. Why are we unable to receive normal downlinks? Does the calss c multicast session disable normal downlinks?).
