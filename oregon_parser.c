#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "oregon_parser.h"

// ==========================================================================
// UTILITY MACROS AND FUNCTIONS (Equivalent to OREGON_hi/lo_nibble, etc.)
// ==========================================================================

// Extracts the high nibble (4 bits) from a byte
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
// Extracts the low nibble from a byte
#define LO_NIBBLE(b) ((b) & 0x0F)

// Converts a Binary Coded Decimal (BCD) byte to its decimal value.
// e.g., 0x21 becomes 21. This replaces Perl's sprintf("%02x", ...) trick.
static inline uint8_t bcd_to_dec(uint8_t bcd) {
    return (uint8_t)((HI_NIBBLE(bcd) * 10) + LO_NIBBLE(bcd));
}

// Sums the high and low nibbles of each byte in an array.
// Corresponds to OREGON_nibble_sum
static int nibble_sum(const uint8_t* bytes, float count) {
    int sum = 0;
    int byte_count = (int)count;
    for (int i = 0; i < byte_count; i++) {
        sum += HI_NIBBLE(bytes[i]);
        sum += LO_NIBBLE(bytes[i]);
    }
    // Handle fractional part for odd nibble counts
    if (count > byte_count) {
        sum += HI_NIBBLE(bytes[byte_count]);
    }
    return sum;
}

// Converts a hex string to a byte array. Returns bytes written or -1 on error.
static int hex_to_bytes(const char* hex_str, uint8_t* out_bytes, size_t max_len) {
    size_t len = strlen(hex_str);
    if (len % 2 != 0) return -1; // Must be even length

    size_t byte_len = len / 2;
    if (byte_len > max_len) return -1; // Buffer too small

    for (size_t i = 0; i < byte_len; i++) {
        if (sscanf(hex_str + 2 * i, "%2hhx", &out_bytes[i]) != 1) {
            return -1; // Invalid hex character
        }
    }
    return (int)byte_len;
}


// ==========================================================================
// DATA EXTRACTION FUNCTIONS (e.g., OREGON_temperature, OREGON_humidity)
// ==========================================================================

static void decode_temperature(const uint8_t* bytes, const char* dev_str, OregonReading* r) {
    strcpy(r->device, dev_str);
    strcpy(r->type, "temperature");
    strcpy(r->units, "C");
    
    // Ternary operator for sign
    double sign = (bytes[6] & 0x08) ? -1.0 : 1.0;
    r->current = sign * (
        (double)bcd_to_dec(bytes[5]) +
        (double)HI_NIBBLE(bytes[4]) / 10.0
    );
}

static void decode_humidity(const uint8_t* bytes, const char* dev_str, OregonReading* r) {
    strcpy(r->device, dev_str);
    strcpy(r->type, "humidity");
    strcpy(r->units, "%");
    r->current = (double)(LO_NIBBLE(bytes[7]) * 10 + HI_NIBBLE(bytes[6]));
    
    const char* comfort_levels[] = {"normal", "comfortable", "dry", "wet"};
    strcpy(r->string_val, comfort_levels[bytes[7] >> 6]);
}

static void decode_simple_battery(const uint8_t* bytes, const char* dev_str, OregonReading* r) {
    strcpy(r->device, dev_str);
    strcpy(r->type, "battery_status");
    // The 3rd bit of the 5th byte (index 4) indicates low battery
    bool is_low = (bytes[4] & 0x04) != 0;
    strcpy(r->string_val, is_low ? "low" : "ok");
}

static void decode_pressure(const uint8_t* bytes, const char* dev_str, int offset, int forecast_nibble, OregonReading* r) {
    strcpy(r->device, dev_str);
    strcpy(r->type, "pressure");
    strcpy(r->units, "hPa");
    r->current = (double)(bytes[8] + offset);

    switch(forecast_nibble) {
        case 0xc: strcpy(r->forecast, "sunny"); break;
        case 0x6: strcpy(r->forecast, "partly"); break;
        case 0x2: strcpy(r->forecast, "cloudy"); break;
        case 0x3: strcpy(r->forecast, "rain"); break;
        default:  strcpy(r->forecast, "unknown"); break;
    }
}


// ==========================================================================
// CHECKSUM FUNCTIONS
// ==========================================================================

static bool checksum1(const uint8_t* b) {
    uint8_t c = HI_NIBBLE(b[6]) + (LO_NIBBLE(b[7]) << 4);
    uint8_t s = (nibble_sum(b, 6.5) - 0xa) & 0xff;
    return s == c;
}

static bool checksum2(const uint8_t* b) {
    uint8_t c = b[8];
    uint8_t s = (nibble_sum(b, 8) - 0xa) & 0xff;
    return s == c;
}

static bool checksum4(const uint8_t* b) {
    uint8_t c = b[9];
    uint8_t s = (nibble_sum(b, 9) - 0xa) & 0xff;
    return s == c;
}

static bool checksum5(const uint8_t* b) {
    uint8_t c = b[10];
    uint8_t s = (nibble_sum(b, 10) - 0xa) & 0xff;
    return s == c;
}

// ... other checksum functions can be added here as needed ...


// ==========================================================================
// SENSOR DECODING METHODS (The main parsers for each sensor type)
// ==========================================================================

// Forward declarations for function pointers
struct SensorType;
typedef OregonReading* (*method_func_t)(const struct SensorType* sensor, const uint8_t* bytes, int* num_readings);

typedef struct SensorType {
    uint32_t key;
    const char* part_name;
    bool (*checksum_func)(const uint8_t* bytes);
    method_func_t method_func;
} SensorType;

// Generic device string generator
static void get_device_string(char* buf, size_t len, const char* part_name, const uint8_t* bytes) {
    snprintf(buf, len, "%s_%02x", part_name, bytes[3]);
    if (HI_NIBBLE(bytes[2]) > 0) {
        char channel_buf[10];
        snprintf(channel_buf, sizeof(channel_buf), "_%d", HI_NIBBLE(bytes[2]));
        strncat(buf, channel_buf, len - strlen(buf) - 1);
    }
}

// Corresponds to OREGON_common_temphydro
static OregonReading* method_common_temphydro(const SensorType* sensor, const uint8_t* bytes, int* num_readings) {
    *num_readings = 3;
    OregonReading* readings = calloc(*num_readings, sizeof(OregonReading));
    if (!readings) {
        *num_readings = 0;
        return NULL;
    }

    char dev_str[50];
    get_device_string(dev_str, sizeof(dev_str), sensor->part_name, bytes);
    
    decode_temperature(bytes, dev_str, &readings[0]);
    decode_humidity(bytes, dev_str, &readings[1]);
    decode_simple_battery(bytes, dev_str, &readings[2]);
    
    return readings;
}

// Corresponds to OREGON_alt_temphydrobaro
static OregonReading* method_alt_temphydrobaro(const SensorType* sensor, const uint8_t* bytes, int* num_readings) {
    *num_readings = 3; // Temp, humidity, pressure. Battery is separate.
    OregonReading* readings = calloc(*num_readings, sizeof(OregonReading));
    if (!readings) {
        *num_readings = 0;
        return NULL;
    }

    char dev_str[50];
    get_device_string(dev_str, sizeof(dev_str), sensor->part_name, bytes);
    
    decode_temperature(bytes, dev_str, &readings[0]);
    decode_humidity(bytes, dev_str, &readings[1]);
    // Note: The perl code for BTHR918N has a separate decode_percentage_battery,
    // and passes specific offsets to pressure. This is a simplified version.
    decode_pressure(bytes, dev_str, 856, HI_NIBBLE(bytes[9]), &readings[2]);
    // For simplicity, we are not decoding percentage battery here.
    
    return readings;
}

// ... other method functions can be added here ...


// ==========================================================================
// SENSOR DEFINITIONS TABLE (Equivalent to the %types hash)
// ==========================================================================

// Key is generated as: (type << 16) | bits
// Example: type=0xfa28, bits=80 -> 0xfa280050
static const SensorType SENSOR_TYPES[] = {
    {0xfa280050, "THGR810",    checksum2, method_common_temphydro},
    {0xfab80050, "WTGR800_T",  checksum2, method_common_temphydro}, // using common for simplicity
    {0x1a990058, "WTGR800_A",  checksum4, NULL}, // Method not implemented
    {0x1a890058, "WGR800",     checksum4, NULL}, // Method not implemented
    {0xea4c0050, "THWR288A",   checksum1, NULL}, // Method not implemented
    {0xea4c0040, "THN132N",    checksum1, NULL}, // Method not implemented
    {0x1a2d0050, "THGR228N",   checksum2, method_common_temphydro},
    {0x1a3d0050, "THGR918",    checksum2, method_common_temphydro},
    {0x5a6d0058, "BTHR918N",   checksum5, method_alt_temphydrobaro},
    {0xca2c0050, "THGR328N",   checksum2, method_common_temphydro},
};
static const int NUM_SENSOR_TYPES = sizeof(SENSOR_TYPES) / sizeof(SENSOR_TYPES[0]);


// ==========================================================================
// MAIN PARSING LOGIC (Equivalent to OREGON_Parse)
// ==========================================================================

void print_readings(const OregonReading* readings, int count) {
    if (count <= 0 || !readings) return;
    
    printf("--- Decoded Sensor: %s ---\n", readings[0].device);
    for(int i = 0; i < count; i++) {
        const OregonReading* r = &readings[i];
        printf("  - Type: %s\n", r->type);
        if (strlen(r->units) > 0) {
            printf("    Value: %.2f %s\n", r->current, r->units);
        }
        if (strlen(r->string_val) > 0) {
            printf("    State: %s\n", r->string_val);
        }
        if (strlen(r->forecast) > 0) {
            printf("    Forecast: %s\n", r->forecast);
        }
    }
    printf("---------------------------------------\n");
}

void parse_oregon_message(const char* hex_msg) {
    uint8_t msg_bytes[32]; // Buffer for raw bytes
    
    // The first byte of the message is the length in bits.
    // The rest is the payload.
    int num_bytes_total = hex_to_bytes(hex_msg, msg_bytes, sizeof(msg_bytes));

    if (num_bytes_total < 3) {
        fprintf(stderr, "Error: Invalid or too short hex message.\n");
        return;
    }

    int bits = msg_bytes[0];
    const uint8_t* payload = &msg_bytes[1];
    
    uint16_t type_id = (payload[0] << 8) | payload[1];
    
    printf("Received Message: %s\n", hex_msg);
    printf("Parsing... Bits: %d, Sensor Type ID: 0x%04x\n", bits, type_id);
    
    const SensorType* found_sensor = NULL;
    
    // Search for the sensor type, trying different bit lengths like in the Perl script
    for (int b = bits; b >= bits - 8 && b > 0; b -= 4) {
        uint32_t key = ((uint32_t)type_id << 16) | b;
        for (int i = 0; i < NUM_SENSOR_TYPES; i++) {
            if (SENSOR_TYPES[i].key == key) {
                found_sensor = &SENSOR_TYPES[i];
                goto found;
            }
        }
    }
    
found:
    if (!found_sensor) {
        fprintf(stderr, "Error: Unknown sensor type/bit length combination.\n");
        return;
    }

    printf("Found Sensor Definition: %s\n", found_sensor->part_name);

    // 2. Validate Checksum
    if (found_sensor->checksum_func) {
        if (!found_sensor->checksum_func(payload)) {
            fprintf(stderr, "Error: Checksum validation failed!\n");
            return;
        }
        printf("Checksum OK.\n");
    } else {
        printf("Warning: No checksum function defined for this sensor.\n");
    }

    // 3. Decode message
    if (found_sensor->method_func) {
        int num_readings = 0;
        OregonReading* readings = found_sensor->method_func(found_sensor, payload, &num_readings);
        
        if (readings) {
            print_readings(readings, num_readings);
            free(readings); // Free the memory allocated by the method function
        }
    } else {
        printf("Notice: No decoding method implemented for '%s'.\n", found_sensor->part_name);
    }
}