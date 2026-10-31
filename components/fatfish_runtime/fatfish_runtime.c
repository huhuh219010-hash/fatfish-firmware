/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fatfish_runtime.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mosaico_camera_service.h"

static const char *TAG = "fatfish_runtime";

#define FATFISH_RUNTIME_MAX_SUBSCRIBERS 4U

typedef struct {
    bool stop;
    fatfish_runtime_command_t command;
} fatfish_runtime_message_t;

typedef struct {
    fatfish_runtime_event_callback_t callback;
    void *user_ctx;
} fatfish_runtime_subscriber_t;

typedef struct {
    SemaphoreHandle_t lock;
    SemaphoreHandle_t stopped;
    QueueHandle_t queue;
    TaskHandle_t worker;
    fatfish_runtime_event_callback_t event_callback;
    void *user_ctx;
    fatfish_runtime_subscriber_t subscribers[FATFISH_RUNTIME_MAX_SUBSCRIBERS];
    bool initialized;
    bool accepting_commands;
} fatfish_runtime_state_t;

static fatfish_runtime_state_t s_runtime;

static TickType_t timeout_to_ticks(uint32_t timeout_ms)
{
    return timeout_ms == FATFISH_RUNTIME_WAIT_FOREVER
               ? portMAX_DELAY
               : pdMS_TO_TICKS(timeout_ms);
}

static bool command_type_valid(fatfish_runtime_command_type_t type)
{
    return type == FATFISH_RUNTIME_COMMAND_CAMERA_PROBE;
}

static void emit_event(const fatfish_runtime_event_t *event)
{
    fatfish_runtime_event_callback_t primary_callback = NULL;
    void *primary_ctx = NULL;
    fatfish_runtime_subscriber_t subscribers[FATFISH_RUNTIME_MAX_SUBSCRIBERS] = {0};

    if (s_runtime.lock) {
        xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
        primary_callback = s_runtime.event_callback;
        primary_ctx = s_runtime.user_ctx;
        for (size_t i = 0; i < FATFISH_RUNTIME_MAX_SUBSCRIBERS; ++i) {
            subscribers[i] = s_runtime.subscribers[i];
        }
        xSemaphoreGive(s_runtime.lock);
    }

    if (primary_callback) {
        primary_callback(event, primary_ctx);
    }
    for (size_t i = 0; i < FATFISH_RUNTIME_MAX_SUBSCRIBERS; ++i) {
        if (subscribers[i].callback) {
            subscribers[i].callback(event, subscribers[i].user_ctx);
        }
    }
}

static void process_camera_probe(const fatfish_runtime_command_t *command)
{
    fatfish_runtime_event_t event = {
        .type = FATFISH_RUNTIME_EVENT_CAMERA_PROBED,
        .command_type = command->type,
        .request_id = command->request_id,
        .status = ESP_OK,
    };

    mosaico_camera_handle_t camera = NULL;
    event.status = mosaico_camera_service_get_default(&camera);
    if (event.status == ESP_OK) {
        mosaico_camera_info_t info = {0};
        event.status = mosaico_camera_get_info(camera, &info);
        if (event.status == ESP_OK) {
            event.data.camera.available = true;
            event.data.camera.width = info.width;
            event.data.camera.height = info.height;
            event.data.camera.pixel_format = info.pixel_format;
            event.data.camera.buffer_count = info.buffer_count;
        }
    }

    emit_event(&event);
}

static void process_command(const fatfish_runtime_command_t *command)
{
    fatfish_runtime_event_t started = {
        .type = FATFISH_RUNTIME_EVENT_COMMAND_STARTED,
        .command_type = command->type,
        .request_id = command->request_id,
        .status = ESP_OK,
    };
    emit_event(&started);

    switch (command->type) {
    case FATFISH_RUNTIME_COMMAND_CAMERA_PROBE:
        process_camera_probe(command);
        break;
    default:
        emit_event(&(fatfish_runtime_event_t) {
            .type = FATFISH_RUNTIME_EVENT_COMMAND_FAILED,
            .command_type = command->type,
            .request_id = command->request_id,
            .status = ESP_ERR_NOT_SUPPORTED,
        });
        break;
    }
}

static void runtime_worker(void *arg)
{
    (void)arg;
    emit_event(&(fatfish_runtime_event_t) {
        .type = FATFISH_RUNTIME_EVENT_READY,
        .status = ESP_OK,
    });

    fatfish_runtime_message_t message;
    while (xQueueReceive(s_runtime.queue, &message, portMAX_DELAY) == pdTRUE) {
        if (message.stop) {
            break;
        }
        process_command(&message.command);
    }

    emit_event(&(fatfish_runtime_event_t) {
        .type = FATFISH_RUNTIME_EVENT_STOPPED,
        .status = ESP_OK,
    });
    xSemaphoreGive(s_runtime.stopped);
    vTaskDelete(NULL);
}

esp_err_t fatfish_runtime_init(const fatfish_runtime_config_t *config)
{
    fatfish_runtime_config_t resolved = FATFISH_RUNTIME_DEFAULT_CONFIG();
    if (config) {
        resolved = *config;
        if (resolved.queue_depth == 0) {
            resolved.queue_depth = 8;
        }
        if (resolved.task_stack_size == 0) {
            resolved.task_stack_size = 4096;
        }
        if (resolved.task_priority == 0) {
            resolved.task_priority = 5;
        }
    }

    if (!s_runtime.lock) {
        s_runtime.lock = xSemaphoreCreateMutex();
        if (!s_runtime.lock) {
            return ESP_ERR_NO_MEM;
        }
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    if (s_runtime.initialized) {
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_INVALID_STATE;
    }

    s_runtime.queue = xQueueCreate(resolved.queue_depth,
                                   sizeof(fatfish_runtime_message_t));
    s_runtime.stopped = xSemaphoreCreateBinary();
    if (!s_runtime.queue || !s_runtime.stopped) {
        if (s_runtime.queue) {
            vQueueDelete(s_runtime.queue);
        }
        if (s_runtime.stopped) {
            vSemaphoreDelete(s_runtime.stopped);
        }
        s_runtime.queue = NULL;
        s_runtime.stopped = NULL;
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_NO_MEM;
    }

    s_runtime.event_callback = resolved.event_callback;
    s_runtime.user_ctx = resolved.user_ctx;
    memset(s_runtime.subscribers, 0, sizeof(s_runtime.subscribers));
    s_runtime.accepting_commands = true;
    s_runtime.initialized = true;
    BaseType_t created = xTaskCreate(runtime_worker, "fatfish_runtime",
                                     resolved.task_stack_size, NULL,
                                     resolved.task_priority, &s_runtime.worker);
    if (created != pdPASS) {
        s_runtime.initialized = false;
        s_runtime.accepting_commands = false;
        s_runtime.event_callback = NULL;
        s_runtime.user_ctx = NULL;
        vQueueDelete(s_runtime.queue);
        vSemaphoreDelete(s_runtime.stopped);
        s_runtime.queue = NULL;
        s_runtime.stopped = NULL;
        s_runtime.worker = NULL;
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(s_runtime.lock);

    ESP_LOGI(TAG, "Runtime started: queue=%lu stack=%lu priority=%lu",
             (unsigned long)resolved.queue_depth,
             (unsigned long)resolved.task_stack_size,
             (unsigned long)resolved.task_priority);
    return ESP_OK;
}

esp_err_t fatfish_runtime_deinit(void)
{
    if (!s_runtime.lock) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    if (!s_runtime.initialized || !s_runtime.accepting_commands ||
        xTaskGetCurrentTaskHandle() == s_runtime.worker) {
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_INVALID_STATE;
    }

    s_runtime.accepting_commands = false;
    fatfish_runtime_message_t message = {.stop = true};
    if (xQueueSend(s_runtime.queue, &message, portMAX_DELAY) != pdTRUE) {
        s_runtime.accepting_commands = true;
        xSemaphoreGive(s_runtime.lock);
        return ESP_FAIL;
    }
    xSemaphoreGive(s_runtime.lock);

    xSemaphoreTake(s_runtime.stopped, portMAX_DELAY);

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    vQueueDelete(s_runtime.queue);
    vSemaphoreDelete(s_runtime.stopped);
    s_runtime.queue = NULL;
    s_runtime.stopped = NULL;
    s_runtime.worker = NULL;
    s_runtime.event_callback = NULL;
    s_runtime.user_ctx = NULL;
    memset(s_runtime.subscribers, 0, sizeof(s_runtime.subscribers));
    s_runtime.initialized = false;
    xSemaphoreGive(s_runtime.lock);
    return ESP_OK;
}

esp_err_t fatfish_runtime_submit(const fatfish_runtime_command_t *command,
                                 uint32_t timeout_ms)
{
    if (!command || !command_type_valid(command->type)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_runtime.lock) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    if (!s_runtime.initialized || !s_runtime.accepting_commands) {
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_INVALID_STATE;
    }

    fatfish_runtime_message_t message = {
        .command = *command,
    };
    BaseType_t sent = xQueueSend(s_runtime.queue, &message,
                                 timeout_to_ticks(timeout_ms));
    xSemaphoreGive(s_runtime.lock);
    return sent == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t fatfish_runtime_submit_camera_probe(uint32_t request_id,
                                              uint32_t timeout_ms)
{
    return fatfish_runtime_submit(&(fatfish_runtime_command_t) {
        .type = FATFISH_RUNTIME_COMMAND_CAMERA_PROBE,
        .request_id = request_id,
    }, timeout_ms);
}

esp_err_t fatfish_runtime_subscribe(
    fatfish_runtime_event_callback_t event_callback, void *user_ctx)
{
    if (!event_callback || !s_runtime.lock) {
        return event_callback ? ESP_ERR_INVALID_STATE : ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    if (!s_runtime.initialized || !s_runtime.accepting_commands) {
        xSemaphoreGive(s_runtime.lock);
        return ESP_ERR_INVALID_STATE;
    }
    for (size_t i = 0; i < FATFISH_RUNTIME_MAX_SUBSCRIBERS; ++i) {
        if (s_runtime.subscribers[i].callback == event_callback &&
            s_runtime.subscribers[i].user_ctx == user_ctx) {
            xSemaphoreGive(s_runtime.lock);
            return ESP_OK;
        }
    }
    for (size_t i = 0; i < FATFISH_RUNTIME_MAX_SUBSCRIBERS; ++i) {
        if (!s_runtime.subscribers[i].callback) {
            s_runtime.subscribers[i] = (fatfish_runtime_subscriber_t) {
                .callback = event_callback,
                .user_ctx = user_ctx,
            };
            xSemaphoreGive(s_runtime.lock);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_runtime.lock);
    return ESP_ERR_NO_MEM;
}

esp_err_t fatfish_runtime_unsubscribe(
    fatfish_runtime_event_callback_t event_callback, void *user_ctx)
{
    if (!event_callback || !s_runtime.lock) {
        return event_callback ? ESP_ERR_INVALID_STATE : ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    for (size_t i = 0; i < FATFISH_RUNTIME_MAX_SUBSCRIBERS; ++i) {
        if (s_runtime.subscribers[i].callback == event_callback &&
            s_runtime.subscribers[i].user_ctx == user_ctx) {
            s_runtime.subscribers[i] = (fatfish_runtime_subscriber_t) {0};
            xSemaphoreGive(s_runtime.lock);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_runtime.lock);
    return ESP_ERR_NOT_FOUND;
}

bool fatfish_runtime_is_initialized(void)
{
    if (!s_runtime.lock) {
        return false;
    }

    xSemaphoreTake(s_runtime.lock, portMAX_DELAY);
    const bool initialized = s_runtime.initialized;
    xSemaphoreGive(s_runtime.lock);
    return initialized;
}
