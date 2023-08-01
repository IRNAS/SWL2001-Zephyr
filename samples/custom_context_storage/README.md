# Custom context storage sample

This sample is the same as the lorawan sample, but uses a custom context storage implementation.
This is selected using `CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL=y`.
The HAL implementation will not use Zephyr settings subsystem to store the context, but instead
will call the user provided functions to store and retrieve the context.
