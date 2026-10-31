/*
 * SPDX-FileCopyrightText: 2026 HoloPartner contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "holo_partner_actions.h"
#include "holo_partner_objects.h"
#include "mosaic_app_catalog.h"
#include "mosaic_hub_actions.h"

#define HOLO_PARTNER_APP_ID 45U

static const mosaic_app_route_t s_holo_partner_routes[] = {
    {
        .action_id = GSP_ACT_ID_HOLO_BACK,
        .target_name = "mosaic-hub",
    },
};

const mosaic_app_descriptor_t mosaic_holo_partner_app = {
    .id = HOLO_PARTNER_APP_ID,
    .launch_action = GSP_ACT_ID_APP_HOLO_PARTNER,
    .back_action = MOSAIC_APP_SHELL_BACK_ACTION,
    .routes = s_holo_partner_routes,
    .route_count = sizeof(s_holo_partner_routes) /
                   sizeof(s_holo_partner_routes[0]),
    .name = "holo_partner",
    .title = "HoloPartner",
    .directory = &gsp_obj_directory_holo_partner,
    .disable_swipe = true,
    /* This first milestone owns its complete canvas, including Back. */
    .custom_shell = true,
};
