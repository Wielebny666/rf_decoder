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

#include "driver/rmt.h"
#include "esp_log.h"
#include "esp_err.h"

#include "rf_tool.h"
#include "rf_timings.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    rf_scrambler_t parent;
    uint32_t flags;
    uint32_t pulse_duration_ticks;
    uint32_t cursor;
    uint32_t bit_cnt;
    uint32_t buffer_size;
    rmt_item32_t buffer[0];
} rfid_scrambler_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *TAG = "rfid_scrambler";

/**********************
 *      MACROS
 **********************/
#define CHECK(a, ret_val, str, ...)                                           \
    if (!(a))                                                                 \
    {                                                                         \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return ret_val;                                                       \
    }

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
inline esp_err_t rfid_scrambler_make_logic(rf_scrambler_t *scrambler, bool value)
{
    //CHECK(scrambler, ESP_ERR_INVALID_ARG, "scrambler pointer can't be null");

    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);

    if (!(rfid_scrambler->bit_cnt % 2))
    {
        rfid_scrambler->buffer[rfid_scrambler->cursor].level0 = value;
        rfid_scrambler->buffer[rfid_scrambler->cursor].duration0 = rfid_scrambler->pulse_duration_ticks;
    }
    else
    {
        rfid_scrambler->buffer[rfid_scrambler->cursor].level1 = value;
        rfid_scrambler->buffer[rfid_scrambler->cursor].duration1 = rfid_scrambler->pulse_duration_ticks;
        rfid_scrambler->cursor++;
    }

    rfid_scrambler->bit_cnt++;
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_pasue(rf_scrambler_t *scrambler)
{
    //CHECK(scrambler, ESP_ERR_INVALID_ARG, "scrambler pointer can't be null");

    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);

    rfid_scrambler_make_logic(scrambler, false);
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_byte(rf_scrambler_t *scrambler, void *value, uint8_t len)
{
    //ESP_LOGD(TAG, "%s", __FUNCTION__);
    CHECK(len, ESP_ERR_INVALID_ARG, "len can't be null");
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);

    uint8_t *pattern = (uint8_t *)value;
    uint8_t point = len;

    while (point)
    {
        //ESP_LOGD(TAG, "val %x - "BYTE_TO_BINARY_PATTERN, *(pattern + len - point), BYTE_TO_BINARY(*(pattern + len - point)));
        for (uint8_t i = 0; i < 8; i++)
        {
            bool bit_value = *(pattern + len - point) & (1 << (7 - i));
            rfid_scrambler_make_logic(scrambler, bit_value);
        }
        point--;
    }
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_carrier_burst(rf_scrambler_t *scrambler, uint8_t carrier_burst_len)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    CHECK(carrier_burst_len, ESP_ERR_INVALID_ARG, "carrier_burst_len can't be null");
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);

    uint8_t carrier_burst_pattern = 0xFF;

    while (carrier_burst_len)
    {
        rfid_scrambler_make_byte(scrambler, &carrier_burst_pattern, sizeof(uint8_t));
        carrier_burst_len--;
    }
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_preamble(rf_scrambler_t *scrambler, uint8_t preamble_pattern, uint8_t preamble_len)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);

    while (preamble_len)
    {
        rfid_scrambler_make_byte(scrambler, &preamble_pattern, sizeof(uint8_t));
        preamble_len--;
    }
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_sync(rf_scrambler_t *scrambler, uint8_t *sync, uint8_t sync_len)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);

    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);
    rfid_scrambler_make_byte(scrambler, sync, sync_len);
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_payload(rf_scrambler_t *scrambler, uint8_t *payload, uint8_t payload_len)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);
    rfid_scrambler_make_byte(scrambler, payload, payload_len);
    return ESP_OK;
}

static esp_err_t rfid_scrambler_make_crc(rf_scrambler_t *scrambler, uint16_t crc)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);
    rfid_scrambler_make_byte(scrambler, &crc, sizeof(crc));
    return ESP_OK;
}

static esp_err_t rfid_scrambler_build_frame(rf_scrambler_t *scrambler, rf_frame_t rf_frame)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    //CHECK(rf_frame.preamble_len, ESP_ERR_INVALID_ARG, "preamble_len can't be null");
    //CHECK(rf_frame.payload_len, ESP_ERR_INVALID_ARG, "payload_len can't be null");

    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);
    rfid_scrambler->cursor = 0;
    rfid_scrambler->bit_cnt = 0;
    rfid_scrambler_make_carrier_burst(scrambler, rf_frame.carrier_burst_len);
    rfid_scrambler_make_pasue(scrambler);
    rfid_scrambler_make_preamble(scrambler, rf_frame.preamble_pattern, rf_frame.preamble_len);
    rfid_scrambler_make_sync(scrambler, rf_frame.sync, rf_frame.sync_len);
    rfid_scrambler_make_payload(scrambler, rf_frame.payload, rf_frame.payload_len);
    rfid_scrambler_make_crc(scrambler, rf_frame.crc);
    return ESP_OK;
}

static esp_err_t rfid_scrambler_get_result(rf_scrambler_t *scrambler, void *result, uint32_t *length)
{
    //ESP_LOGD(TAG, "%s", __FUNCTION__);
    CHECK(result && length, ESP_ERR_INVALID_ARG, "result and length can't be null");
    rfid_scrambler_t *rfid_scrambler = __containerof(scrambler, rfid_scrambler_t, parent);
    *(rmt_item32_t **)result = rfid_scrambler->buffer;
    *length = rfid_scrambler->cursor;
    return ESP_OK;
}

static esp_err_t rfid_scrambler_del(rf_scrambler_t *scrambler)
{
    rfid_scrambler_t *rfid_parser = __containerof(scrambler, rfid_scrambler_t, parent);
    free(rfid_parser);
    return ESP_OK;
}

rf_scrambler_t *rfid_scrambler_create(const rf_scrambler_config_t *config)
{
    rf_scrambler_t *ret = NULL;

    uint32_t builder_size = sizeof(rfid_scrambler_t) + config->buffer_size * sizeof(rmt_item32_t);
    rfid_scrambler_t *rfid_scrambler = calloc(1, builder_size);
    CHECK(rfid_scrambler, ret, "ALLOC MEM FAIL");

    rfid_scrambler->buffer_size = config->buffer_size;
    rfid_scrambler->flags = config->flags;

    uint8_t counter_clk_div;
    rmt_get_clk_div((rmt_channel_t)config->dev_hdl, &counter_clk_div);

    uint32_t counter_clk_hz = APB_CLK_FREQ / counter_clk_div;
    float ratio = (float)counter_clk_hz / 1e6;
    uint32_t duration = (uint32_t)(ratio * RFID_PULSE_DURATION_US);
    CHECK((duration < 0x7FFF), ret, "CHANGE DIV CLOCK");

    rfid_scrambler->pulse_duration_ticks = duration;
    //rfid_scrambler->parent.make_logic = rfid_scrambler_make_logic;
    rfid_scrambler->parent.make_preamble = rfid_scrambler_make_preamble;
    rfid_scrambler->parent.make_sync = rfid_scrambler_make_sync;
    rfid_scrambler->parent.make_payload = rfid_scrambler_make_payload;
    rfid_scrambler->parent.make_crc = rfid_scrambler_make_crc;
    rfid_scrambler->parent.build_frame = rfid_scrambler_build_frame;
    rfid_scrambler->parent.get_result = rfid_scrambler_get_result;
    rfid_scrambler->parent.del = rfid_scrambler_del;

    return &rfid_scrambler->parent;
}