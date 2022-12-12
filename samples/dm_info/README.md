# DM Info

This sample is based on `SWSD001/apps/examples/dm_info`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

Once a network is joined (i.e. when the corresponding event is triggered), the application will initialize a file transfer by calling `smtc_modem_file_upload_init()` and start it by calling `smtc_modem_file_upload_start()`.

When the transfer is over, the event `SMTC_MODEM_EVENT_UPLOADDONE` will be triggered with a status indicating if LoRa Cloud acknowledged the reception (`SMTC_MODEM_EVENT_UPLOADDONE_SUCCESSFUL`) or not (`SMTC_MODEM_EVENT_UPLOADDONE_ABORTED`).

The application can relaunch the same file transfer as soon as the current one is over.
The file upload can be stopped anytime by calling `smtc_modem_file_upload_reset()`.
