#ifndef ONE_WIRE_H
#define ONE_WIRE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The 1-Wire driver structure.
 * 
 * The structure and corresponding functions implement
 * base ROM commands to simplify 1-Wire device usage.
 */
typedef struct {
    void* user_data;
    uint8_t (*reset)(void* user_data);
    uint8_t (*read_bit)(void* user_data);
    void (*write_bit)(void* user_data, uint8_t data);

    // global search state
    unsigned char _rom_no[8];
    uint8_t _last_discrepancy;
    uint8_t _last_family_discrepancy;
    bool _last_device_flag;
} one_wire_t;

/**
 * Initialize one_wire_t structure.
 *
 * @param one_wire
 * @param user_data user data for reset, read_bit and write_bit function
 * @param reset a function that performs the 1-Wire reset function. It should return 1 if a device asserts a presence pulse, 0 otherwise.
 * @param read_bit a function that read 1 bit from 1-Wire interface. If received bit is 1, then function should return 0, otherwise 1.
 * @param write_bit a function that read 1 bit from 1-Wire interface.
 */
void one_wire_init(one_wire_t* one_wire, void* user_data, uint8_t (*reset)(void* user_data), uint8_t (*read_bit)(void* user_data), void (*write_bit)(void* user_data, uint8_t data));

/**
 * Perform a 1-Wire reset cycle.
 *
 * @param one_wire
 * @return 1 if a device responds, otherwise 0.
 */
uint8_t one_wire_reset(one_wire_t* one_wire);

/**
 * Issue a 1-Wire rom select command.
 * 
 * @param one_wire_t
 * @param rom device address
 */
void one_wire_select(one_wire_t* one_wire, const uint8_t rom[8]);

/**
 * Issue a 1-Wire rom skip command, to address all on bus.
 * 
 * @param one_wire_t
 */
void one_wire_skip(one_wire_t* one_wire);

/**
 * Write one bit.
 *
 * @param one_wire
 * @param data true of false
 */
void one_wire_write_bit(one_wire_t* one_wire, uint8_t data);

/**
 * Write one byte.
 *
 * @param one_wire
 * @param data byte
 */
void one_wire_write(one_wire_t* one_wire, uint8_t data);

/**
 * Write multiple bytes.
 * 
 * @param one_wire
 * @param buf buffer with bytes
 * @param count buffer size
 */
void one_wire_write_bytes(one_wire_t* one_wire, const uint8_t* buf, uint16_t count);

/**
 * Read one bit.
 * 
 * @param one_wire
 * @return read 1 or 0
 */
uint8_t one_wire_read_bit(one_wire_t* one_wire);

/**
 * Read one byte.
 * 
 * @param one_wire
 * @return read byte
 */
uint8_t one_wire_read(one_wire_t* one_wire);

/**
 * Read mutiple bytes
 * 
 * @param one_wire
 * @param buf butter for read bytes
 * @param count number bytes to read
 */
void one_wire_read_bytes(one_wire_t* one_wire, uint8_t* buf, uint16_t count);

//
// Device search functions
//

/**
 * Clear the search state so that if will start from the beginning again.
 * 
 * @param one_wire
 */
void one_wire_search_reset(one_wire_t* one_wire);

/**
 * Setup the search to find the device type 'family_code' on the next call
 * to one_wire_search(one_wire, *new_addr) if it is present.
 */
void one_wire_search_target(one_wire_t* one_wire, uint8_t family_code);

/**
 * Look for the next device. 
 * 
 * Returns 1 if a new address has been returned.
 * A zero might mean that the bus is shorted, there are
 * no devices, or you have already retrieved all of them.
 * 
 * The order is deterministic. You will always get the same devices in the same order.
 * 
 * @param one_wire
 * @param new_addr array to save next device addres.
 * @return 1 if a new address has been found, otherwise 0 value.
 */
uint8_t one_wire_search(one_wire_t* one_wire, uint8_t new_addr[8]);

#ifdef __cplusplus
}
#endif
#endif // OneWire_h
