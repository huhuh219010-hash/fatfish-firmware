/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Logical filesystem roots owned by the Fatfish runtime. */
typedef enum {
    FATFISH_PATH_DATA = 0,
    FATFISH_PATH_SYSTEM,
    FATFISH_PATH_ROOT_MAX,
} fatfish_path_root_t;

typedef esp_err_t (*fatfish_path_space_provider_t)(void *ctx,
                                                   uint64_t *total_bytes,
                                                   uint64_t *free_bytes);

/** Set the physical mount point for a logical root during early boot. */
esp_err_t fatfish_paths_set(fatfish_path_root_t root, const char *path);

/** Return the configured mount point, or NULL when the root is not set. */
const char *fatfish_paths_get(fatfish_path_root_t root);

/** Register or clear the storage-space provider for a logical root. */
esp_err_t fatfish_paths_set_space_provider(
    fatfish_path_root_t root, fatfish_path_space_provider_t provider,
    void *ctx);

/** Query total and free bytes through the registered provider. */
esp_err_t fatfish_paths_get_space(fatfish_path_root_t root,
                                  uint64_t *total_bytes,
                                  uint64_t *free_bytes);

/** Compose a path below a configured logical root. */
esp_err_t fatfish_paths_join(fatfish_path_root_t root, const char *subpath,
                             char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
