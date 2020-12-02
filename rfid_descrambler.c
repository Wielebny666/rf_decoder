/*
 * rf_decoder.c
 *
 *  Created on: 12 pa� 2020
 *      Author: kurza
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <sys/cdefs.h>

#include "esp_log.h"
#include "driver/rmt.h"

#include "rf_tool.h"
#include "rf_timings.h"

/*********************
 *      DEFINES
 *********************/
#define RFID_MAX_FRAME_RMT_WORDS (20)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    rf_descrambler_t parent;
    uint32_t flags;
    uint32_t logic_ticks;
    uint32_t logic1_ticks;
    uint32_t margin_ticks;
    rmt_item32_t *buffer;
    uint32_t cursor;
    uint32_t bit_cnt;
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
#define CHECK(a, ret_val, str, ...)                                           \
    if (!(a))                                                                 \
    {                                                                         \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return ret_val;                                                       \
    }

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static inline bool rfid_check_in_range(uint32_t raw_ticks, uint32_t target_ticks, uint32_t margin_ticks)
{
    return (raw_ticks < (target_ticks + margin_ticks)) && (raw_ticks > (target_ticks - margin_ticks));
}

static bool rfid_descrambler_logic_lvl(rfid_descrambler_t *descrambler)
{
    bool ret;
    rmt_item32_t item = descrambler->buffer[descrambler->cursor];
    if (!(descrambler->bit_cnt % 2))
    {
        ret = (item.level0 == true) && rfid_check_in_range(item.duration0, descrambler->logic_ticks, descrambler->margin_ticks);
    }
    else
    {
        ret = (item.level1 == true) && rfid_check_in_range(item.duration1, descrambler->logic_ticks, descrambler->margin_ticks);
        descrambler->cursor += 1;
    }

    descrambler->bit_cnt++;
    return ret;
}

static esp_err_t rfid_descrambler_logic(rf_descrambler_t *descrambler, bool *logic)
{
    esp_err_t ret = ESP_FAIL;
    bool logic_value = false;
    rfid_descrambler_t *rfid_parser = __containerof(descrambler, rfid_descrambler_t, parent);

    logic_value = rfid_descrambler_logic_lvl(rfid_parser);
    ret = ESP_OK;

    if (ret == ESP_OK)
    {
        *logic = logic_value;
    }

    return ret;
}

static esp_err_t rfid_descrambler_input(rf_descrambler_t *descrambler, void *raw_data, uint32_t length)
{
    CHECK(raw_data, ESP_ERR_INVALID_ARG, "input data can't be null");
    esp_err_t ret = ESP_OK;
    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);
    rfid_descrambler->buffer = raw_data;

    if (length > RFID_MAX_FRAME_RMT_WORDS)
    {
        ret = ESP_FAIL;
    }
    return ret;
}

static esp_err_t rfid_descrambler_get_scan_code(rf_descrambler_t *descrambler, uint8_t *signal)
{
    esp_err_t ret = ESP_FAIL;
    uint32_t descr_signal = 0;
    bool logic_value = false;

    CHECK(signal, ESP_ERR_INVALID_ARG, "can't be null");

    rfid_descrambler_t *rfid_parser = __containerof(descrambler, rfid_descrambler_t, parent);

    if (rfid_descrambler_logic(descrambler, &logic_value) == ESP_OK)
    {
    }
    return ret;
}

static esp_err_t rfid_descrambler_del(rf_descrambler_t *descrambler)
{
    rfid_descrambler_t *rfid_parser = __containerof(descrambler, rfid_descrambler_t, parent);
    free(rfid_parser);
    return ESP_OK;
}

rf_descrambler_t *rfid_descrambler_create(const rf_descrambler_config_t *config)
{
    rf_descrambler_t *ret = NULL;
    rfid_descrambler_t *rfid_descrambler = calloc(1, sizeof(rfid_descrambler_t));

    rfid_descrambler->flags = config->flags;

    uint32_t counter_clk_hz = 80000000 / 2;
    //rmt_get_counter_clock((rmt_channel_t)config->dev_hdl, &counter_clk_hz);
    float ratio = (float)counter_clk_hz / 1e6;
    rfid_descrambler->pulse_duration_ticks = (uint32_t)(ratio * RFID_PULSE_DURATION_US);
    rfid_descrambler->margin_ticks = (uint32_t)(ratio * config->margin_us);
    rfid_descrambler->parent.input = rfid_descrambler_input;
    rfid_descrambler->parent.get_scan_code = rfid_descrambler_get_scan_code;
    rfid_descrambler->parent.del = rfid_descrambler_del;
    return &rfid_descrambler->parent;

    return ret;
}
