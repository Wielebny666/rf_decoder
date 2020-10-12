/*
 * rf_decoder.c
 *
 *  Created on: 12 paü 2020
 *      Author: kurza
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <sys/cdefs.h>

#include "esp_log.h"
#include "driver/rmt.h"

#include "rf_common.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
	rf_decoder_t parent;
	uint32_t flags;
	uint32_t pulse_duration_ticks;
	uint32_t margin_ticks;
	rmt_item32_t *buffer;
	uint32_t buffer_len;
} rfid_decoder_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *TAG = "rf_decoder";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static esp_err_t rf_decoder_input_data(rf_decoder_t *decoder, void *raw_data, uint32_t lenght)
{
	esp_err_t ret = ESP_OK;

	return ret;
}

static esp_err_t rf_decoder_del()
{
	esp_err_t ret = ESP_OK;

	return ret;
}

rf_decoder_t *rf_decoder_create(const rf_decoder_config_t *config)
{
	rf_decoder_t *ret = NULL;

    return ret;
}
