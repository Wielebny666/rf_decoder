/*
 * rf_decoder.c
 *
 *  Created on: 12 pa� 2020
 *      Author: kurza
 */

/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "driver/rmt.h"
#include "esp_log.h"
#include "esp_err.h"

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
    uint32_t margin_ticks;
    rmt_item32_t *buffer;
    uint32_t buffer_len;
    uint32_t cursor;
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
static inline bool rfid_check_in_range(rfid_descrambler_t *descrambler, uint32_t duration)
{
    return (duration < (descrambler->logic_ticks + descrambler->margin_ticks)) && (duration > (descrambler->logic_ticks - descrambler->margin_ticks));
}

static inline bool rfid_check_bits_in_range(rfid_descrambler_t *descrambler, uint32_t duration, uint8_t *cnt)
{
    CHECK(cnt, ESP_ERR_INVALID_ARG, "cnt can't be null");

    for (uint8_t i = 1; i <= 8; i++)
    {
        if ((duration < (descrambler->logic_ticks * i + descrambler->margin_ticks)) && (duration > (descrambler->logic_ticks * i - descrambler->margin_ticks)))
        {
            *cnt = i;
            return true;
        }
    }
    *cnt = 0;
    return false;
}

static inline bool rfid_check_bits_in_range_2(rfid_descrambler_t *descrambler, uint32_t duration, uint8_t *cnt)
{
    CHECK(cnt, ESP_ERR_INVALID_ARG, "cnt can't be null");

    *cnt = duration / descrambler->logic_ticks;

    if ((duration < (descrambler->logic_ticks * *cnt + descrambler->margin_ticks)) && (duration > (descrambler->logic_ticks * *cnt - descrambler->margin_ticks)))
    {
        return true;
    }

    *cnt = 0;
    return false;
}

static esp_err_t rfid_descrambler_input(rf_descrambler_t *descrambler, void *raw_data, uint32_t length)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);
    CHECK(raw_data, ESP_ERR_INVALID_ARG, "input data can't be null");
    ESP_LOGD(TAG, "RMT length %d", length);

    esp_err_t ret = ESP_OK;
    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);
    rfid_descrambler->buffer = raw_data;
    rfid_descrambler->buffer_len = length;

    if (length > RFID_MAX_FRAME_RMT_WORDS)
    {
        //ret = ESP_FAIL;
    }
    return ret;
}

static esp_err_t rfid_descrambler_get_scan_code(rf_descrambler_t *descrambler, uint8_t *signal)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t descramble_signal = 0;

    CHECK(signal, ESP_ERR_INVALID_ARG, "can't be null");

    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);
    for (int i = 0; i < rfid_descrambler->buffer_len; i++)
    {
        if (rfid_check_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration0))
        {
            descramble_signal <<= 1;
            descramble_signal |= rfid_descrambler->buffer[i].level0;
            if (rfid_check_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration1))
            {
                descramble_signal <<= 1;
                descramble_signal |= rfid_descrambler->buffer[i].level1;
            }
            else
            {
                ESP_LOGD(TAG, "Error at %d", i);
                ESP_LOGD(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
                ESP_LOGD(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
                return ret;
            }
        }
        else
        {
            ESP_LOGD(TAG, "Error at %d", i);
            ESP_LOGD(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
            ESP_LOGD(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
            return ret;
        }

        if (i > 0 && !(i % 7))
        {
            *signal = descramble_signal;
            signal++;
            descramble_signal = 0;
        }
    }
    ret = ESP_OK;
    return ret;
}

static esp_err_t rfid_descrambler_get_raw_frame(rf_descrambler_t *descrambler, uint8_t *signal)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t descramble_signal = 0;
    uint8_t cnt = 0;
    uint8_t cursor = 0;
    CHECK(signal, ESP_ERR_INVALID_ARG, "can't be null");

    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);

    rfid_descrambler->cursor = 0;

    for (int i = 0; i < rfid_descrambler->buffer_len; i++)
    {
        if (rfid_check_bits_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration0, &cnt))
        {
            while (cnt--)
            {
                //ESP_LOGD(TAG, "%d - %d duration0: %d cnt: %d", i, rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0, cnt);
                descramble_signal <<= 1;
                descramble_signal |= rfid_descrambler->buffer[i].level0;

                if (cursor == 7)
                {
                    *signal = descramble_signal;
                    signal++;
                    descramble_signal = 0;
                    cursor = 0;
                }
                else
                {
                    cursor++;
                }
            }

            if (rfid_check_bits_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration1, &cnt))
            {

                while (cnt--)
                {
                    //ESP_LOGD(TAG, "%d - %d duration1: %d cnt: %d", i, rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1, cnt);
                    descramble_signal <<= 1;
                    descramble_signal |= rfid_descrambler->buffer[i].level1;

                    if (cursor == 7)
                    {
                        *signal = descramble_signal;
                        signal++;
                        descramble_signal = 0;
                        cursor = 0;
                    }
                    else
                    {
                        cursor++;
                    }
                }
            }
            else if (rfid_descrambler->buffer[i].duration1 == 0)
            {
                descramble_signal <<= 1;
                descramble_signal |= rfid_descrambler->buffer[i].level1;

                *signal = descramble_signal;
                ESP_LOGD(TAG, "End...");
                break;
            }
            else
            {
                if (i < 4)
                {
                    descramble_signal = 0;
                    cursor = 0;
                    i = 3;
                    continue;
                }
                ESP_LOGE(TAG, "Error at %d", i);
                ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
                ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
                return ESP_FAIL;
            }
        }
        else
        {
            if (i < 4)
            {
                descramble_signal = 0;
                cursor = 0;
                i = 3;
                continue;
            }
            ESP_LOGE(TAG, "Error at %d", i);
            ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
            ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
            return ESP_FAIL;
        }
    }
    ret = ESP_OK;
    return ret;
}

static esp_err_t rfid_descrambler_get_scan_data(rf_descrambler_t *descrambler, uint8_t *signal, size_t signal_len, uint16_t sync, uint8_t *data, size_t data_len)
{
    ESP_LOGD(TAG, "pointer %p", signal);
    uint8_t *test = memmem(signal, signal_len, &sync, sizeof(sync));
    ESP_LOGD(TAG, "pointer %p", test);

    return ESP_OK;
}

static esp_err_t rfid_descrambler_get_scan_frame(rf_descrambler_t *descrambler, uint8_t *signal, size_t *signal_size)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t descramble_signal = 0;
    uint8_t cnt = 0;
    uint8_t cursor = 0;
    *signal_size = 0;

    CHECK((signal || signal_size), ESP_ERR_INVALID_ARG, "can't be null");

    rfid_descrambler_t *rfid_descrambler = __containerof(descrambler, rfid_descrambler_t, parent);

    rfid_descrambler->cursor = 0;

    for (int i = 0; i < rfid_descrambler->buffer_len; i++)
    {
        if (rfid_check_bits_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration0, &cnt))
        {
            while (cnt--)
            {
                //ESP_LOGD(TAG, "%d - %d duration0: %d cnt: %d", i, rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0, cnt);
                descramble_signal <<= 1;
                descramble_signal |= rfid_descrambler->buffer[i].level0;

                if (cursor == 7)
                {
                    *signal = descramble_signal;
                    signal++;
                    (*signal_size)++;
                    descramble_signal = 0;
                    cursor = 0;
                }
                else
                {
                    cursor++;
                }
            }

            if (rfid_check_bits_in_range(rfid_descrambler, rfid_descrambler->buffer[i].duration1, &cnt))
            {
                while (cnt--)
                {
                    //ESP_LOGD(TAG, "%d - %d duration1: %d cnt: %d", i, rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1, cnt);
                    descramble_signal <<= 1;
                    descramble_signal |= rfid_descrambler->buffer[i].level1;

                    if (cursor == 7)
                    {
                        *signal = descramble_signal;
                        signal++;
                        (*signal_size)++;
                        descramble_signal = 0;
                        cursor = 0;
                    }
                    else
                    {
                        cursor++;
                    }
                }
            }
            else if (rfid_descrambler->buffer[i].duration1 == 0)
            {
                descramble_signal <<= 1;
                descramble_signal |= rfid_descrambler->buffer[i].level1;

                *signal = descramble_signal;
                ESP_LOGD(TAG, "End...");
                break;
            }
            else
            {
                if (i < 4)
                {
                    descramble_signal = 0;
                    cursor = 0;
                    i = 3;
                    continue;
                }
                ESP_LOGE(TAG, "Error at %d", i);
                ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
                ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
                return ESP_FAIL;
            }
        }
        else
        {
            if (i < 4)
            {
                descramble_signal = 0;
                cursor = 0;
                i = 3;
                continue;
            }
            ESP_LOGE(TAG, "Error at %d", i);
            ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level0, rfid_descrambler->buffer[i].duration0);
            ESP_LOGE(TAG, "level0: %d duration0: %d", rfid_descrambler->buffer[i].level1, rfid_descrambler->buffer[i].duration1);
            return ESP_FAIL;
        }
    }
    ret = ESP_OK;
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

    uint8_t counter_clk_div;
    rmt_get_clk_div((rmt_channel_t)config->dev_hdl, &counter_clk_div);
    ESP_LOGD(TAG, "RMT clk div %d", counter_clk_div);
    uint32_t counter_clk_hz = APB_CLK_FREQ / counter_clk_div;
    float ratio = (float)counter_clk_hz / 1e6;

    uint32_t duration = (uint32_t)(ratio * RFID_PULSE_DURATION_US);
    CHECK((duration < 0x7FFF), ret, "CHANGE DIV CLOCK");
    ESP_LOGD(TAG, "duration %d", duration);

    rfid_descrambler->logic_ticks = (uint32_t)(ratio * RFID_PULSE_DURATION_US);
    rfid_descrambler->margin_ticks = (uint32_t)(ratio * config->margin_us);
    rfid_descrambler->parent.input = rfid_descrambler_input;
    // rfid_descrambler->parent.get_scan_raw_frame = rfid_descrambler_get_raw_frame;
    rfid_descrambler->parent.get_scan_raw_frame = rfid_descrambler_get_scan_frame;
    
    rfid_descrambler->parent.get_scan_data = rfid_descrambler_get_scan_data;
    rfid_descrambler->parent.del = rfid_descrambler_del;
    return &rfid_descrambler->parent;

    return ret;
}
