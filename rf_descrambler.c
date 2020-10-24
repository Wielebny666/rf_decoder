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
    rf_descrambler_t parent;
    uint32_t flags;
    uint32_t pulse_duration_ticks;
    uint32_t margin_ticks;
    rmt_item32_t *buffer;
    uint32_t buffer_len;
} rfid_descrambler_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *TAG = "rfid_descrambler";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static esp_err_t rfid_descrambler_input(rf_descrambler_t *descrambler, void *raw_data, uint32_t lenght)
{
    esp_err_t ret = ESP_OK;
    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);

    return ret;
}

static esp_err_t rfid_descrambler_get_scan_code(rf_descrambler_t *descrambler)
{
    esp_err_t ret = ESP_FAIL;

    rfid_descrambler_t *nec_parser = __containerof(descrambler, rfid_descrambler_t, parent);
    return ret;
}

static esp_err_t rfid_descrambler_del(rf_descrambler_t *descrambler)
{
    rfid_descrambler_t *rfid_parser = __containerof(descrambler, rfid_descrambler_t, parent);
    free(rfid_parser);
    return ESP_OK;
}

rf_descrambler_t* rfid_descrambler_create(const rf_descrambler_config_t *config)
{
    rf_descrambler_t *ret = NULL;
    rfid_descrambler_t *rfid_descrambler = calloc(1, sizeof(rfid_descrambler_t));

    rfid_descrambler->flags = config->flags;

    uint32_t counter_clk_hz = 0;
    rmt_get_counter_clock((rmt_channel_t) config->dev_hdl, &counter_clk_hz);
    float ratio = (float) counter_clk_hz / 1e6;

    rfid_descrambler->margin_ticks = (uint32_t) (ratio * config->margin_us);
    rfid_descrambler->parent.input = rfid_descrambler_input;
    rfid_descrambler->parent.get_scan_code = rfid_descrambler_get_scan_code;
    rfid_descrambler->parent.del = rfid_descrambler_del;
    return &rfid_descrambler->parent;

    return ret;
}
