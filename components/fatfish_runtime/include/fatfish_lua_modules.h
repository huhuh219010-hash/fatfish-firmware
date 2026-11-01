/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *module_id;
    const char *display_name;
} fatfish_lua_module_info_t;

typedef esp_err_t (*fatfish_lua_module_register_fn)(const char *fatfs_base_path);

typedef struct {
    const char *module_id;
    const char *display_name;
    fatfish_lua_module_register_fn reg;
} fatfish_lua_module_external_t;

/**
 * @brief Register an application-provided Lua module for Fatfish startup.
 *
 *        Call this before fatfish_runtime_lua_init(). The module participates in
 *        enabled_lua_modules using module_id, alongside built-in Fatfish Lua
 *        modules.
 */
esp_err_t fatfish_lua_modules_register_external(
    const fatfish_lua_module_external_t *module);
esp_err_t fatfish_lua_modules_register(const char *enabled_modules,
                                       const char *fatfs_base_path);
esp_err_t fatfish_lua_modules_get_compiled_modules(
    const fatfish_lua_module_info_t **modules, size_t *count);

#ifdef __cplusplus
}
#endif
