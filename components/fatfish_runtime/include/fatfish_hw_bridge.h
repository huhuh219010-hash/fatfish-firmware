/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* Product-owned hardware arbitration bridge used by Fatfish Lua modules. */

#include <stdbool.h>
#include "claw_hw_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FATFISH_HW_GPIO,
    FATFISH_HW_I2C_BUS,
    FATFISH_HW_I2C_ADDR,
    FATFISH_HW_I2S_BUS,
    FATFISH_HW_SPI_BUS,
    FATFISH_HW_UART_BUS,
    FATFISH_HW_ADC_UNIT,
    FATFISH_HW_ADC_CHANNEL,
    FATFISH_HW_LEDC_TIMER,
    FATFISH_HW_LEDC_CHANNEL,
    FATFISH_HW_SDMMC_SLOT,
    FATFISH_HW_DSI_BUS,
    FATFISH_HW_LDO_CHANNEL,
} fatfish_hw_resource_kind_t;

typedef struct {
    fatfish_hw_resource_kind_t kind;
    int a;
    int b; /* Address/channel/timer, SDMMC bus width, or zero. */
} fatfish_hw_resource_t;

typedef struct {
    const char *name;
    bool device;
    size_t group; /* Codec/I2S aliases share one physical claim. */
    const fatfish_hw_resource_t *resources;
    size_t resource_count;
    bool board_owned;
} fatfish_hw_group_t;

typedef struct {
    const fatfish_hw_group_t *groups;
    size_t group_count;
} fatfish_hw_board_config_t;

extern const fatfish_hw_board_config_t fatfish_hw_generated_board;
/* Register the catalog before board initialization; no resources are claimed here. */
esp_err_t fatfish_hw_bridge_init(claw_hw_registry_handle_t registry);

#ifdef __cplusplus
}
#endif
