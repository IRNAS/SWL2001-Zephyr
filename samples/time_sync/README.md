# Lorawan time synchronization

This sample is based on `SWSD001/apps/examples/time_sync`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

Once a network is joined (i.e. when the corresponding event is triggered), the configured time synchronization service is initialized by calling `smtc_modem_time_set_sync_interval_s()` and launched by calling `smtc_modem_time_start_sync_service()`. Then, time synchronization requests are sent on a regular basis by this service. The service can be stopped anytime by calling `smtc_modem_time_stop_sync_service()`.

For more information:

* MAC command: refer to the LoRaWAN specification v1.0.4
* ALC sync: refer to the [LoRa Developer portal](https://lora-developers.semtech.com/resources/tools/lora-edge-asset-management/?url=rst/Modem/alc_sync.html).
