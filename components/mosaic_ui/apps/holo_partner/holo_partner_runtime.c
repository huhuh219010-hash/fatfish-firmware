/*
 * SPDX-FileCopyrightText: 2026 HoloPartner contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "holo_partner_runtime.h"

#include <stdbool.h>
#include <stdio.h>

#include "fatfish_runtime.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define HOLO_PARTNER_REQUEST_PREFIX UINT32_C(0x48500000)

static portMUX_TYPE s_result_lock = portMUX_INITIALIZER_UNLOCKED;
static uint32_t s_request_sequence;
static uint32_t s_pending_request_id;
static bool s_result_pending;
static bool s_subscribed;
static fatfish_runtime_event_t s_result;

static void set_status(esp_gsp_handle_t ui, gsp_component_key_t status_bind,
                       const char *text)
{
    if (ui) {
        (void)esp_gsp_set_text(ui, status_bind, text);
    }
}

static void runtime_event(const fatfish_runtime_event_t *event, void *user_ctx)
{
    (void)user_ctx;
    if (!event || event->type != FATFISH_RUNTIME_EVENT_CAMERA_PROBED) {
        return;
    }

    portENTER_CRITICAL(&s_result_lock);
    if (s_pending_request_id != 0U &&
        event->request_id == s_pending_request_id) {
        s_result = *event;
        s_result_pending = true;
    }
    portEXIT_CRITICAL(&s_result_lock);
}

void holo_partner_runtime_start(esp_gsp_handle_t ui,
                                gsp_component_key_t status_bind)
{
    portENTER_CRITICAL(&s_result_lock);
    s_pending_request_id = 0U;
    s_result_pending = false;
    portEXIT_CRITICAL(&s_result_lock);
    set_status(ui, status_bind, "Tap the face to check camera");
}

void holo_partner_runtime_stop(void)
{
    portENTER_CRITICAL(&s_result_lock);
    s_pending_request_id = 0U;
    s_result_pending = false;
    portEXIT_CRITICAL(&s_result_lock);
}

esp_err_t holo_partner_runtime_probe_camera(
    esp_gsp_handle_t ui, gsp_component_key_t status_bind)
{
    if (!fatfish_runtime_is_initialized()) {
        set_status(ui, status_bind, "Runtime is not ready");
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_subscribed) {
        const esp_err_t subscribe_err =
            fatfish_runtime_subscribe(runtime_event, NULL);
        if (subscribe_err != ESP_OK) {
            set_status(ui, status_bind, "Runtime connection failed");
            return subscribe_err;
        }
        s_subscribed = true;
    }

    portENTER_CRITICAL(&s_result_lock);
    if (s_pending_request_id != 0U) {
        portEXIT_CRITICAL(&s_result_lock);
        return ESP_ERR_INVALID_STATE;
    }
    s_request_sequence = (s_request_sequence + 1U) & UINT32_C(0x000FFFFF);
    if (s_request_sequence == 0U) {
        s_request_sequence = 1U;
    }
    const uint32_t request_id =
        HOLO_PARTNER_REQUEST_PREFIX | s_request_sequence;
    s_pending_request_id = request_id;
    s_result_pending = false;
    portEXIT_CRITICAL(&s_result_lock);

    set_status(ui, status_bind, "Checking camera...");
    const esp_err_t err =
        fatfish_runtime_submit_camera_probe(request_id, 0U);
    if (err != ESP_OK) {
        portENTER_CRITICAL(&s_result_lock);
        if (s_pending_request_id == request_id) {
            s_pending_request_id = 0U;
        }
        portEXIT_CRITICAL(&s_result_lock);
        set_status(ui, status_bind, "Camera check could not start");
    }
    return err;
}

void holo_partner_runtime_poll(esp_gsp_handle_t ui,
                               gsp_component_key_t status_bind)
{
    fatfish_runtime_event_t result = {0};
    bool ready = false;

    portENTER_CRITICAL(&s_result_lock);
    if (s_result_pending && s_pending_request_id == s_result.request_id) {
        result = s_result;
        s_result_pending = false;
        s_pending_request_id = 0U;
        ready = true;
    }
    portEXIT_CRITICAL(&s_result_lock);
    if (!ready) {
        return;
    }

    if (result.status == ESP_OK && result.data.camera.available) {
        if (result.data.camera.width > 0U && result.data.camera.height > 0U) {
            char status[64];
            (void)snprintf(status, sizeof(status), "Camera ready: %lux%lu",
                           (unsigned long)result.data.camera.width,
                           (unsigned long)result.data.camera.height);
            set_status(ui, status_bind, status);
        } else {
            set_status(ui, status_bind, "Camera ready");
        }
    } else if (result.status == ESP_ERR_NOT_FOUND) {
        set_status(ui, status_bind, "Camera not detected");
    } else {
        set_status(ui, status_bind, "Camera check failed");
    }
}
