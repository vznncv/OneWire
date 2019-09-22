#include "one_wire.h"

void one_wire_init(one_wire_t* one_wire, void* user_data, uint8_t (*reset)(void* user_data), uint8_t (*read_bit)(void* user_data), void (*write_bit)(void* user_data, uint8_t data))
{
    one_wire->user_data = user_data;
    one_wire->reset = reset;
    one_wire->read_bit = read_bit;
    one_wire->write_bit = write_bit;

    one_wire_search_reset(one_wire);
}

uint8_t one_wire_reset(one_wire_t* one_wire)
{
    return one_wire->reset(one_wire->user_data);
}

void one_wire_select(one_wire_t* one_wire, const uint8_t rom[8])
{
    one_wire_write(one_wire, 0x55);
    one_wire_write_bytes(one_wire, rom, 8);
}

void one_wire_skip(one_wire_t* one_wire)
{
    one_wire_write(one_wire, 0xCC); // Skip ROM
}

void one_wire_write_bit(one_wire_t* one_wire, uint8_t data)
{
    one_wire->write_bit(one_wire->user_data, data);
}

void one_wire_write(one_wire_t* one_wire, uint8_t data)
{
    for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
        one_wire->write_bit(one_wire->user_data, (bit_mask & data) ? 1 : 0);
    }
}

void one_wire_write_bytes(one_wire_t* one_wire, const uint8_t* buf, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        one_wire_write(one_wire, buf[i]);
    }
}

uint8_t one_wire_read_bit(one_wire_t* one_wire)
{
    return one_wire->read_bit(one_wire->user_data);
}

uint8_t one_wire_read(one_wire_t* one_wire)
{
    uint8_t res = 0;
    for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
        if (one_wire->read_bit(one_wire->user_data)) {
            res |= bit_mask;
        }
    }
    return res;
}

void one_wire_read_bytes(one_wire_t* one_wire, uint8_t* buf, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        buf[i] = one_wire_read(one_wire);
    }
}

//
// Device search functions
//

void one_wire_search_reset(one_wire_t* one_wire)
{
    one_wire->_last_discrepancy = 0;
    one_wire->_last_device_flag = false;
    one_wire->_last_family_discrepancy = 0;
    for (int i = 0; i < 8; i++) {
        one_wire->_rom_no[i] = 0;
    }
}

void one_wire_search_target(one_wire_t* one_wire, uint8_t family_code)
{
    one_wire->_last_discrepancy = 64;
    one_wire->_last_device_flag = false;
    one_wire->_last_family_discrepancy = 0;
    one_wire->_rom_no[0] = family_code;
    for (int i = 1; i < 8; i++) {
        one_wire->_rom_no[i] = 0;
    }
}

uint8_t one_wire_search(one_wire_t* one_wire, uint8_t new_addr[8])
{
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number;
    bool search_result;
    uint8_t id_bit, cmp_id_bit;

    unsigned char rom_byte_mask, search_direction;

    // initialize for search
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = false;

    // if the last call was not the last one
    if (!one_wire->_last_device_flag) {
        // 1-Wire reset
        if (!one_wire_reset(one_wire)) {
            // reset the search
            one_wire_search_reset(one_wire);
            return false;
        }

        // issue the search command
        one_wire_write(one_wire, 0xF0); // NORMAL SEARCH

        // loop to do the search
        do {
            // read a bit and its complement
            id_bit = one_wire->read_bit(one_wire->user_data);
            cmp_id_bit = one_wire->read_bit(one_wire->user_data);

            // check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                break;
            } else {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit; // bit write value for search
                } else {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < one_wire->_last_discrepancy) {
                        search_direction = ((one_wire->_rom_no[rom_byte_number] & rom_byte_mask) > 0);
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == one_wire->_last_discrepancy);
                    }
                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0) {
                        last_zero = id_bit_number;

                        // check for Last discrepancy in family
                        if (last_zero < 9) {
                            one_wire->_last_family_discrepancy = last_zero;
                        }
                    }
                }

                // set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_byte_mask
                if (search_direction == 1) {
                    one_wire->_rom_no[rom_byte_number] |= rom_byte_mask;
                } else {
                    one_wire->_rom_no[rom_byte_number] &= ~rom_byte_mask;
                }

                // serial number search direction write bit
                one_wire->write_bit(one_wire->user_data, search_direction);

                // increment the byte counter id_bit_number
                // and shift the mask rom_byte_mask
                id_bit_number++;
                rom_byte_mask <<= 1;

                // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_byte_mask == 0) {
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        } while (rom_byte_number < 8); // loop until through all ROM bytes 0-7

        // if the search was successful then
        if (!(id_bit_number < 65)) {
            // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
            one_wire->_last_discrepancy = last_zero;

            // check for last device
            if (one_wire->_last_discrepancy == 0) {
                one_wire->_last_device_flag = true;
            }
            search_result = true;
        }
    }

    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !one_wire->_rom_no[0]) {
        one_wire_search_reset(one_wire);
        search_result = false;
    } else {
        for (int i = 0; i < 8; i++) {
            new_addr[i] = one_wire->_rom_no[i];
        }
    }
    return search_result;
}
