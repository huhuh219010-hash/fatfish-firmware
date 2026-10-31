/*
 * SPDX-FileCopyrightText: 2026 HoloPartner contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_gsp.h"

void holo_partner_runtime_start(esp_gsp_handle_t ui,
                                gsp_component_key_t status_bind);
void holo_partner_runtime_stop(void);
esp_err_t holo_partner_runtime_probe_camera(
    esp_gsp_handle_t ui, gsp_component_key_t status_bind);
void holo_partner_runtime_poll(esp_gsp_handle_t ui,
                               gsp_component_key_t status_bind);
