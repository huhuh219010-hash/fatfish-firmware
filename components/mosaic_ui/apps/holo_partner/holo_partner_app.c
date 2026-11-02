/*
 * SPDX-FileCopyrightText: 2026 HoloPartner contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "holo_partner_actions.h"
#include "holo_partner_binds.h"
#include "holo_partner_objects.h"
#include "holo_partner_runtime.h"
#include "mosaic_app_catalog.h"
#include "mosaic_hub_actions.h"
#include "mosaic_runtime.h"

#define HOLO_PARTNER_APP_ID 45U

static const mosaic_app_route_t s_holo_partner_routes[] = {
    {
        .action_id = GSP_ACT_ID_HOLO_BACK,
        .target_name = "mosaic-hub",
    },
};

static void *s_probe_timer;

static void holo_partner_probe_tick(esp_gsp_handle_t ui, void *user_ctx)
{
    (void)user_ctx;
    holo_partner_runtime_poll(ui, GSP_BIND_HOLO_PROBE_STATUS);
}

static void holo_partner_started(esp_gsp_handle_t ui)
{
    holo_partner_runtime_start(ui, GSP_BIND_HOLO_PROBE_STATUS);
    s_probe_timer = esp_gsp_timer_create(
        ui, 100U, holo_partner_probe_tick, NULL);
}

static void holo_partner_stopping(esp_gsp_handle_t ui)
{
    if (s_probe_timer) {
        (void)esp_gsp_timer_delete(ui, s_probe_timer);
        s_probe_timer = NULL;
    }
    holo_partner_runtime_stop();
}

static void holo_partner_event(
    esp_gsp_handle_t ui, const struct mosaic_event *event)
{
    if (event && event->type == MOSAIC_EVENT_UI_CALL &&
        event->data.call.action_id == GSP_ACT_ID_HOLO_PROBE) {
        (void)holo_partner_runtime_probe_camera(
            ui, GSP_BIND_HOLO_PROBE_STATUS);
    }
}

const mosaic_app_descriptor_t mosaic_holo_partner_app = {
    .id = HOLO_PARTNER_APP_ID,
    .launch_action = GSP_ACT_ID_APP_HOLO_PARTNER,
    .back_action = MOSAIC_APP_SHELL_BACK_ACTION,
    .routes = s_holo_partner_routes,
    .route_count = sizeof(s_holo_partner_routes) /
                   sizeof(s_holo_partner_routes[0]),
    .name = "holo_partner",
    .title = "Fatfish",
    .directory = &gsp_obj_directory_holo_partner,
    .disable_swipe = true,
    /* This first milestone owns its complete canvas, including Back. */
    .custom_shell = true,
    .on_started = holo_partner_started,
    .on_stopping = holo_partner_stopping,
    .on_event = holo_partner_event,
};
