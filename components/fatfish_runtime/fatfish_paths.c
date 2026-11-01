/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "fatfish_paths.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"

#define FATFISH_PATH_MAX_LEN 32U

static const char *TAG = "fatfish_paths";

static char s_root_dirs[FATFISH_PATH_ROOT_MAX][FATFISH_PATH_MAX_LEN];
static fatfish_path_space_provider_t s_space_providers[FATFISH_PATH_ROOT_MAX];
static void *s_space_provider_ctx[FATFISH_PATH_ROOT_MAX];

esp_err_t fatfish_paths_set(fatfish_path_root_t root, const char *path)
{
    ESP_RETURN_ON_FALSE((unsigned)root < FATFISH_PATH_ROOT_MAX,
                        ESP_ERR_INVALID_ARG, TAG, "bad root %d", (int)root);
    ESP_RETURN_ON_FALSE(path && path[0], ESP_ERR_INVALID_ARG, TAG,
                        "empty path");
    ESP_RETURN_ON_FALSE(
        strlcpy(s_root_dirs[root], path, sizeof(s_root_dirs[root])) <
            sizeof(s_root_dirs[root]),
        ESP_ERR_INVALID_SIZE, TAG, "root path too long: %s", path);
    s_space_providers[root] = NULL;
    s_space_provider_ctx[root] = NULL;
    return ESP_OK;
}

const char *fatfish_paths_get(fatfish_path_root_t root)
{
    if ((unsigned)root >= FATFISH_PATH_ROOT_MAX || !s_root_dirs[root][0]) {
        return NULL;
    }
    return s_root_dirs[root];
}

esp_err_t fatfish_paths_set_space_provider(
    fatfish_path_root_t root, fatfish_path_space_provider_t provider,
    void *ctx)
{
    ESP_RETURN_ON_FALSE((unsigned)root < FATFISH_PATH_ROOT_MAX,
                        ESP_ERR_INVALID_ARG, TAG, "bad root %d", (int)root);
    s_space_providers[root] = provider;
    s_space_provider_ctx[root] = provider ? ctx : NULL;
    return ESP_OK;
}

esp_err_t fatfish_paths_get_space(fatfish_path_root_t root,
                                  uint64_t *total_bytes,
                                  uint64_t *free_bytes)
{
    ESP_RETURN_ON_FALSE((unsigned)root < FATFISH_PATH_ROOT_MAX,
                        ESP_ERR_INVALID_ARG, TAG, "bad root %d", (int)root);
    ESP_RETURN_ON_FALSE(total_bytes && free_bytes, ESP_ERR_INVALID_ARG, TAG,
                        "space output is NULL");
    ESP_RETURN_ON_FALSE(s_space_providers[root], ESP_ERR_INVALID_STATE, TAG,
                        "space provider for root %d is not set", (int)root);
    return s_space_providers[root](s_space_provider_ctx[root], total_bytes,
                                   free_bytes);
}

esp_err_t fatfish_paths_join(fatfish_path_root_t root, const char *subpath,
                             char *out, size_t out_size)
{
    const char *base = fatfish_paths_get(root);
    int written;

    ESP_RETURN_ON_FALSE(base, ESP_ERR_INVALID_STATE, TAG, "root %d not set",
                        (int)root);
    ESP_RETURN_ON_FALSE(out && out_size, ESP_ERR_INVALID_ARG, TAG,
                        "bad out buffer");

    if (subpath && subpath[0]) {
        written = snprintf(out, out_size, "%s/%s", base, subpath);
    } else {
        written = snprintf(out, out_size, "%s", base);
    }
    ESP_RETURN_ON_FALSE(
        written >= 0 && (size_t)written < out_size, ESP_ERR_INVALID_SIZE, TAG,
        "joined path too long: %s/%s", base, subpath ? subpath : "");
    return ESP_OK;
}

/* ABI compatibility for retained ESP-Claw modules. The project's build drops
 * claw_core/src/claw_paths.c, so these symbols share the Fatfish-owned state. */
esp_err_t claw_paths_set(fatfish_path_root_t root, const char *path)
{
    return fatfish_paths_set(root, path);
}

const char *claw_paths_get(fatfish_path_root_t root)
{
    return fatfish_paths_get(root);
}

esp_err_t claw_paths_set_space_provider(
    fatfish_path_root_t root, fatfish_path_space_provider_t provider,
    void *ctx)
{
    return fatfish_paths_set_space_provider(root, provider, ctx);
}

esp_err_t claw_paths_get_space(fatfish_path_root_t root,
                               uint64_t *total_bytes,
                               uint64_t *free_bytes)
{
    return fatfish_paths_get_space(root, total_bytes, free_bytes);
}

esp_err_t claw_paths_join(fatfish_path_root_t root, const char *subpath,
                          char *out, size_t out_size)
{
    return fatfish_paths_join(root, subpath, out, out_size);
}
