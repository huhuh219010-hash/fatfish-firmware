/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOSAIC_SETUP_MODE_ONBOARDING = 0,
    MOSAIC_SETUP_MODE_MANAGE,
} mosaic_setup_mode_t;

typedef enum {
    MOSAIC_SETUP_ROUTE_OVERVIEW = 0,
    MOSAIC_SETUP_ROUTE_NETWORK,
    MOSAIC_SETUP_ROUTE_LLM,
    MOSAIC_SETUP_ROUTE_INTEGRATIONS,
} mosaic_setup_route_t;

typedef enum {
    MOSAIC_SETUP_REGION_UNKNOWN = 0,
    MOSAIC_SETUP_REGION_MAINLAND,
    MOSAIC_SETUP_REGION_INTERNATIONAL,
} mosaic_setup_region_t;

typedef struct {
    /** Start background region detection lasting at most timeout_ms. */
    esp_err_t (*detect)(void *user_ctx, uint32_t timeout_ms);
    /** Persist the region the user picked after detection failed. */
    esp_err_t (*select)(void *user_ctx, mosaic_setup_region_t region);
    void *user_ctx;
} mosaic_setup_region_ops_t;

typedef esp_err_t (*mosaic_setup_app_request_cb_t)(
    void *user_ctx, const char *app_name);

/** Provide App navigation for hosts that do not use the ESP loader.
 *
 * Passing NULL clears the override. Firmware normally uses the built-in
 * loader fallback; host and Web simulators should bind their active runtime.
 */
esp_err_t mosaic_setup_configure_app_request(
    mosaic_setup_app_request_cb_t callback, void *user_ctx);

/** Open Setup Center at one logical route. */
esp_err_t mosaic_setup_open(mosaic_setup_mode_t mode,
                            mosaic_setup_route_t route);

/** Return to this App when a managed Setup route closes; NULL clears it. */
esp_err_t mosaic_setup_set_return_app(const char *app_name);

/** Connect onboarding to the platform's region service. */
esp_err_t mosaic_setup_configure_region(
    const mosaic_setup_region_ops_t *ops);

/** Thread-safe publication of the current device region. */
esp_err_t mosaic_setup_set_region(mosaic_setup_region_t region);

#ifdef __cplusplus
}
#endif
