# Basic 1-Wire interface driver

The library contains hardware independent C driver for 1-Wire interface. It's based on an arduino [OneWire](https://github.com/PaulStoffregen/OneWire) and inherits base driver functionality.


To use driver, 3 low-level functions should implemented:

1. `reset` - send reset signal to 1-Wire
2. `read_bit` - read bit from 1-Wire
3. `write_bit` - write bit to 1-Wire

## Example

The example contains snippet of the DS18B20 sensor usage with 1-Wire interface:

```
...

void delay_us(uint32_t value) {
    // hardware specific implementation ...
}

/**
 * Read temperature form DS18B20 sensor.
 * 
 * @param one_wire one wire driver interface
 * @param rom sensor rom (address)
 * @param temp output temperature (celsius degrees)
 * @return 0 on success, otherwise non-zero value
 */
int read_ds18b20_temp(one_wire_t* one_wire, uint8_t rom[8], float* temp)
{
    // 1. run conversion
    one_wire_reset(one_wire);
    one_wire_select(one_wire, rom);
    one_wire_write(one_wire, 0x44);

    // 2. wait conversion
    delay_us(1000);

    // 3. read scratchpad (memory)
    uint8_t data[9];
    one_wire_reset(one_wire);
    one_wire_select(one_wire, rom);
    one_wire_write(one_wire, 0xBE);
    one_wire_read_bytes(one_wire, data, 9);

    // 4. convert data
    int16_t raw = (data[1] << 8) | data[0];
    *temp = raw / 16.0f;

    return 0;
}


static uint8_t one_wire_impl_reset(void* user_data) {
   // hardware specific implementation ...
}

static void one_wire_impl_write_bit(void* user_data, uint8_t data) {
   // hardware specific implementation ...
}

static uint8_t one_wire_stm32f3_impl_read_bit(void* user_data) {
   // hardware specific implementation ...
}




int main() {

    // some initialization code
    ...


    // initialize one wire interface
    const uint8_t DS18B20_CODE = 0x28;
    static one_wire_t one_wire_obj;
    one_wire_t* one_wire = &one_wire_obj;
    void *user_data = ...;
    one_wire_init(one_wire, user_data, one_wire_impl_reset, one_wire_stm32f3_impl_read_bit, one_wire_impl_write_bit);

    // detect devices
    int ret_code;
    const int max_devices = 12;
    int device_count = 0;
    uint8_t device_ids[max_devices][8] = { 0 };

    ret_code = one_wire_reset(one_wire);
    if (!ret_code) {
        Error_Handler();
    }
    printf("Devices have been found.\n");

    one_wire_search_target(one_wire, DS18B20_CODE);
    while (one_wire_search(one_wire, device_ids[device_count])) {
        device_count++;
        if (device_count >= max_devices) {
            break;
        }
    }
    printf("Total devices: %i\n", device_count);
    printf("Device ids:\n");
    for (int i = 0; i < device_count; i++) {
        printf(" - ");
        for (int j = 0; j < 8; j++) {
            int value = device_ids[i][j];
            printf("%02X", value);
        }
        printf("\n");
    }


    // read temperature
    while (1) {
        delay_us(1000000);
        printf("Sensor temperature:\n");
        for (int i = 0; i < device_count; i++) {
            float temp = 0.0;
            read_ds18b20_temp(one_wire, device_ids[i], &temp);
            printf("%i. %.2f C\n", i, temp);
        }
    }
}


...
```
