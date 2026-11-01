/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "fatfish_runtime.h"

#include <stdio.h>

#include "app_registry.h"
#include "audio_hub.h"
#include "cap_app_mgr.h"
#include "cap_lua.h"
#include "cap_system.h"
#include "claw_cap.h"
#include "esp_check.h"
#include "esp_log.h"
#include "fatfish_lua_modules.h"
#include "fatfish_paths.h"

static const char *TAG = "fatfish_runtime_lua";
static bool s_lua_initialized;
static fatfish_runtime_app_registry_changed_cb_t s_app_changed_callback;
static void *s_app_changed_user_ctx;

static const fatfish_runtime_capability_info_t s_capabilities[] = {
#if CONFIG_APP_CLAW_CAP_LUA
    {"cap_lua", "Lua", true},
#endif
#if CONFIG_APP_CLAW_CAP_SYSTEM
    {"cap_system", "System", true},
#endif
};

static void start_audio_service(void)
{
    static const audio_hub_config_t config = {
        .mixer = {
            .sample_rate = 16000,
            .channels = 1,
            .bits = 16,
            .frame_ms = 20,
            .system_full_gain = 1.0f,
            .app_full_gain = 1.0f,
            .app_ducked_gain = 0.3f,
            .duck_release_ms = 300,
            .output_volume = 80,
        },
        .capture = {
            .sample_rate = 16000,
            .channels = 1,
            .bits = 16,
            .frame_ms = 20,
            .ring_frames = 8,
        },
    };
    esp_err_t err = audio_hub_start(&config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "audio service unavailable: %s", esp_err_to_name(err));
    }
}

static esp_err_t init_app_registry(void)
{
    char data_apps[96];
    char system_apps[96];

    ESP_RETURN_ON_ERROR(
        fatfish_paths_join(FATFISH_PATH_DATA, "apps", data_apps,
                           sizeof(data_apps)),
        TAG, "resolve writable App directory failed");
    ESP_RETURN_ON_ERROR(
        fatfish_paths_join(FATFISH_PATH_SYSTEM, "apps", system_apps,
                           sizeof(system_apps)),
        TAG, "resolve system App directory failed");

    ESP_RETURN_ON_ERROR(app_registry_init(), TAG,
                        "initialize App registry failed");
    ESP_RETURN_ON_ERROR(app_registry_add_directory(data_apps), TAG,
                        "add writable App directory failed");
    ESP_RETURN_ON_ERROR(app_registry_add_directory(system_apps), TAG,
                        "add system App directory failed");
    return app_registry_reload();
}

esp_err_t fatfish_runtime_lua_register_app_changed_cb(
    fatfish_runtime_app_registry_changed_cb_t callback, void *user_ctx)
{
    ESP_RETURN_ON_FALSE(callback, ESP_ERR_INVALID_ARG, TAG,
                        "App registry callback is required");
    ESP_RETURN_ON_FALSE(!s_app_changed_callback, ESP_ERR_INVALID_STATE, TAG,
                        "App registry callback is already configured");

    s_app_changed_callback = callback;
    s_app_changed_user_ctx = user_ctx;
    if (s_lua_initialized) {
        return app_registry_register_changed_cb(callback, user_ctx);
    }
    return ESP_OK;
}

esp_err_t fatfish_runtime_lua_init(
    const fatfish_runtime_lua_config_t *config)
{
    const char *data_root;
    const char *system_root;
    char package_path[96];

    ESP_RETURN_ON_FALSE(config && config->hardware_registry,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Lua hardware registry is required");
    ESP_RETURN_ON_FALSE(fatfish_runtime_is_initialized(),
                        ESP_ERR_INVALID_STATE, TAG,
                        "Fatfish runtime is not initialized");
    ESP_RETURN_ON_FALSE(!s_lua_initialized, ESP_ERR_INVALID_STATE, TAG,
                        "Lua runtime is already initialized");

    data_root = fatfish_paths_get(FATFISH_PATH_DATA);
    system_root = fatfish_paths_get(FATFISH_PATH_SYSTEM);
    ESP_RETURN_ON_FALSE(data_root && system_root, ESP_ERR_INVALID_STATE, TAG,
                        "Fatfish storage roots are not configured");

    start_audio_service();

    ESP_RETURN_ON_ERROR(cap_lua_set_hw_registry(config->hardware_registry),
                        TAG, "bind Lua hardware registry failed");

    snprintf(package_path, sizeof(package_path), "%s/scripts/builtin",
             system_root);
    ESP_RETURN_ON_ERROR(cap_lua_add_package_path_dir(package_path), TAG,
                        "add builtin Lua package path failed");
    snprintf(package_path, sizeof(package_path), "%s/scripts/builtin/lib",
             system_root);
    ESP_RETURN_ON_ERROR(cap_lua_add_package_path_dir(package_path), TAG,
                        "add builtin Lua library path failed");

    ESP_RETURN_ON_ERROR(
        fatfish_lua_modules_register(config->enabled_modules, data_root), TAG,
        "register Lua modules failed");

    ESP_RETURN_ON_ERROR(init_app_registry(), TAG,
                        "initialize Fatfish Apps failed");
    if (s_app_changed_callback) {
        ESP_RETURN_ON_ERROR(
            app_registry_register_changed_cb(s_app_changed_callback,
                                             s_app_changed_user_ctx),
            TAG, "register App listener failed");
    }

    ESP_RETURN_ON_ERROR(claw_cap_init(), TAG,
                        "initialize capability runtime failed");
    ESP_RETURN_ON_ERROR(cap_lua_register_group(), TAG,
                        "register Lua capability group failed");
    ESP_RETURN_ON_ERROR(cap_app_mgr_register_group(), TAG,
                        "register App capability group failed");
#if CONFIG_APP_CLAW_CAP_SYSTEM
    ESP_RETURN_ON_ERROR(cap_system_register_group(), TAG,
                        "register System capability group failed");
#endif
    ESP_RETURN_ON_ERROR(claw_cap_start_all(), TAG,
                        "start Fatfish Lua capabilities failed");
#if CONFIG_APP_CLAW_CAP_SYSTEM
    ESP_RETURN_ON_ERROR(
        cap_system_time_sync_service_start(
            &(cap_system_time_sync_service_config_t) {
                .network_ready = config->network_ready,
                .network_ready_ctx = config->network_ready_user_ctx,
                .on_sync_success = NULL,
            }),
        TAG, "start system time synchronization failed");
#endif
    s_lua_initialized = true;
    ESP_LOGI(TAG, "Lua and hardware modules initialized by Fatfish runtime");
    return ESP_OK;
}

bool fatfish_runtime_lua_is_initialized(void)
{
    return s_lua_initialized;
}

esp_err_t fatfish_runtime_get_compiled_capabilities(
    const fatfish_runtime_capability_info_t **groups, size_t *count)
{
    ESP_RETURN_ON_FALSE(groups && count, ESP_ERR_INVALID_ARG, TAG,
                        "capability output is required");
    *groups = s_capabilities;
    *count = sizeof(s_capabilities) / sizeof(s_capabilities[0]);
    return ESP_OK;
}
