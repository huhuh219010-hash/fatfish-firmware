/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "claw_hw_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FATFISH_RUNTIME_WAIT_FOREVER UINT32_MAX

#define FATFISH_RUNTIME_DEFAULT_CONFIG() { \
    .event_callback = NULL,             \
    .user_ctx = NULL,                   \
    .queue_depth = 8,                   \
    .task_stack_size = 4096,            \
    .task_priority = 5,                 \
}

typedef enum {
    FATFISH_RUNTIME_COMMAND_CAMERA_PROBE = 1,
} fatfish_runtime_command_type_t;

typedef struct {
    fatfish_runtime_command_type_t type;
    uint32_t request_id;
} fatfish_runtime_command_t;

typedef enum {
    FATFISH_RUNTIME_EVENT_READY = 1,
    FATFISH_RUNTIME_EVENT_COMMAND_STARTED,
    FATFISH_RUNTIME_EVENT_CAMERA_PROBED,
    FATFISH_RUNTIME_EVENT_COMMAND_FAILED,
    FATFISH_RUNTIME_EVENT_STOPPED,
} fatfish_runtime_event_type_t;

typedef struct {
    bool available;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint8_t buffer_count;
} fatfish_runtime_camera_info_t;

typedef struct {
    fatfish_runtime_event_type_t type;
    fatfish_runtime_command_type_t command_type;
    uint32_t request_id;
    esp_err_t status;
    union {
        fatfish_runtime_camera_info_t camera;
    } data;
} fatfish_runtime_event_t;

/**
 * @brief Runtime event callback
 *
 * The callback runs on the fatfish_runtime worker task. It should return
 * quickly and must not call fatfish_runtime_deinit().
 */
typedef void (*fatfish_runtime_event_callback_t)(
    const fatfish_runtime_event_t *event, void *user_ctx);

typedef struct {
    fatfish_runtime_event_callback_t event_callback;
    void *user_ctx;
    uint32_t queue_depth;
    uint32_t task_stack_size;
    uint32_t task_priority;
} fatfish_runtime_config_t;

typedef struct {
    claw_hw_registry_handle_t hardware_registry;
    const char *enabled_modules;
} fatfish_runtime_lua_config_t;

/** @brief Start the singleton runtime and its command worker. */
esp_err_t fatfish_runtime_init(const fatfish_runtime_config_t *config);

/** @brief Bind hardware and register the retained Lua modules once. */
esp_err_t fatfish_runtime_lua_init(
    const fatfish_runtime_lua_config_t *config);

bool fatfish_runtime_lua_is_initialized(void);

/** @brief Stop the worker and release runtime resources. */
esp_err_t fatfish_runtime_deinit(void);

/** @brief Copy a command into the runtime queue. */
esp_err_t fatfish_runtime_submit(const fatfish_runtime_command_t *command,
                                 uint32_t timeout_ms);

/** @brief Submit the first read-only hardware command. */
esp_err_t fatfish_runtime_submit_camera_probe(uint32_t request_id,
                                              uint32_t timeout_ms);

/** @brief Subscribe to runtime events without replacing the primary callback. */
esp_err_t fatfish_runtime_subscribe(
    fatfish_runtime_event_callback_t event_callback, void *user_ctx);

/** @brief Remove a previously registered runtime event subscription. */
esp_err_t fatfish_runtime_unsubscribe(
    fatfish_runtime_event_callback_t event_callback, void *user_ctx);

bool fatfish_runtime_is_initialized(void);

#ifdef __cplusplus
}
#endif
