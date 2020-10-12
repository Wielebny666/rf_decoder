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
typedef struct rf_coder_s rf_coder_t;
typedef struct rf_decoder_s rf_decoder_t;

struct rf_coder_s
{

};

struct rf_decoder_s
{
	esp_err_t (*input)(rf_decoder_t *parser, void *raw_data, uint32_t length);
    esp_err_t (*get_scan_code)(rf_decoder_t *parser, uint32_t *address, uint32_t *command, bool *repeat);
    esp_err_t (*del)(rf_decoder_t *parser);
};

typedef struct
{
	rf_dev_t dev_hdl;
	uint32_t flags;
	uint32_t buffer_size;
}rf_coder_config_t;

typedef struct
{
	rf_dev_t dev_hdl;
	uint32_t flags;
	uint32_t margin_us;
}rf_decoder_config_t;

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
