# Lorawan stream

This sample is based on `SWSD001/apps/examples/stream`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

Once a network is joined (i.e. when the corresponding event is triggered), the application will configure a stream and periodically add data to the stream buffer. This periodic action is based on the LoRa Basics Modem alarm functionality. Each time the alarm-related event is triggered, the application adds a chunk of data to the stream buffer.

In this example, pieces of a pangram (*"The quick brown fox jumps over the lazy dog"*) are streamed.
