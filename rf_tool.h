/*
 * rf_common.h
 *
 *  Created on: 12 pa� 2020
 *      Author: kurza
 */

#ifndef RF_COMMON_H
#define RF_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
// #include "esp_err.h"

/*********************
 *      DEFINES
 *********************/
#define RF_SCRAMBLER_DEFAULT_CONFIG(dev) \
    {                                    \
        .dev_hdl = dev,                  \
        .flags = 0,                      \
        .buffer_size = 128,              \
    }

#define RF_DESCRAMBLER_DEFAULT_CONFIG(dev) \
    {                                      \
        .dev_hdl = dev,                    \
        .flags = 0,                        \
        .margin_us = 60,                   \
    }

/**********************
 *      TYPEDEFS
 **********************/
typedef void *rf_dev_t;
typedef struct rf_scrambler_s rf_scrambler_t;
typedef struct rf_descrambler_s rf_descrambler_t;

typedef struct
{
    uint8_t carrier_burst_len;
    uint8_t preamble_pattern;
    uint8_t preamble_len;
    uint8_t *sync;
    uint8_t sync_len;
    uint8_t *payload;
    uint8_t payload_len;
    uint16_t crc;
} rf_frame_t;

struct rf_scrambler_s
{
    esp_err_t (*make_logic)(rf_scrambler_t *scrambler, bool value);
    esp_err_t (*make_preamble)(rf_scrambler_t *scrambler, uint8_t preamble_pattern, uint8_t preamble_len);
    esp_err_t (*make_sync)(rf_scrambler_t *scrambler, uint8_t *sync, uint8_t sync_len);
    esp_err_t (*make_payload)(rf_scrambler_t *scrambler, uint8_t *payload, uint8_t payload_len);
    esp_err_t (*make_crc)(rf_scrambler_t *scrambler, uint16_t crc);
    esp_err_t (*build_frame)(rf_scrambler_t *scrambler, rf_frame_t rf_frame);
    esp_err_t (*get_result)(rf_scrambler_t *scrambler, void *result, uint32_t *length);
    esp_err_t (*del)(rf_scrambler_t *scrambler);
};

struct rf_descrambler_s
{
    esp_err_t (*input)(rf_descrambler_t *descrambler, void *raw_data, uint32_t length);
    esp_err_t (*get_scan_code)(rf_descrambler_t *descrambler, uint8_t *signal);
    esp_err_t (*del)(rf_descrambler_t *decoder);
};

typedef struct
{
    rf_dev_t dev_hdl;
    uint32_t flags;
    uint32_t buffer_size;
} rf_scrambler_config_t;

typedef struct
{
    rf_dev_t dev_hdl;
    uint32_t flags;
    uint32_t margin_us;
} rf_descrambler_config_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
rf_scrambler_t *rfid_scrambler_create(const rf_scrambler_config_t *config);
rf_descrambler_t *rfid_descrambler_create(const rf_descrambler_config_t *config);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RF_COMMON_H */
