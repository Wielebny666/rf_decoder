/*
 * rf_common.h
 *
 *  Created on: 12 paü 2020
 *      Author: kurza
 */

#ifndef RF_COMMON_H_
#define RF_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include "esp_err.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef void *rf_dev_t;
typedef struct rf_scrambler_s rf_scrambler_t;
typedef struct rf_descrambler_s rf_descrambler_t;

struct rf_scrambler_s
{
    esp_err_t (*make_preamble)(rf_scrambler_t *scrambler);
    esp_err_t (*make_sync)(rf_scrambler_t *scrambler);
    esp_err_t (*make_payload)(rf_scrambler_t *scrambler);
    esp_err_t (*make_crc)(rf_scrambler_t *scrambler);
    esp_err_t (*get_result)(rf_scrambler_t*scrambler, void *result, uint32_t *length);
    esp_err_t (*del)(rf_scrambler_t *scrambler);
};

struct rf_descrambler_s
{
    esp_err_t (*input)(rf_descrambler_t *descrambler, void *raw_data, uint32_t length);
    esp_err_t (*get_scan_code)(rf_descrambler_t *descrambler);
    esp_err_t (*del)(rf_descrambler_t *decoder);
};

typedef struct
{
    rf_dev_t dev_hdl;
    uint32_t flags;
    uint32_t buffer_size;
}rf_scrambler_config_t;

typedef struct
{
    rf_dev_t dev_hdl;
    uint32_t flags;
    uint32_t margin_us;
}rf_descrambler_config_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* COMPONENTS_RF_DECODER_RF_COMMON_H_ */
