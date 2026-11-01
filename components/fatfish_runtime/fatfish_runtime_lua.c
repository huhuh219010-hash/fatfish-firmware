/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "fatfish_runtime.h"

#include <stdio.h>

#include "cap_lua.h"
#include "esp_check.h"
#include "esp_log.h"
#include "fatfish_lua_modules.h"
#include "fatfish_paths.h"

static const char *TAG = "fatfish_runtime_lua";
static bool s_lua_initialized;

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

    s_lua_initialized = true;
    ESP_LOGI(TAG, "Lua and hardware modules initialized by Fatfish runtime");
    return ESP_OK;
}

bool fatfish_runtime_lua_is_initialized(void)
{
    return s_lua_initialized;
}
