/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FATFISH_STATE_STOPPED = 0,
    FATFISH_STATE_IDLE,
} fatfish_state_t;

typedef struct {
    fatfish_state_t current;
    fatfish_state_t previous;
    uint64_t entered_at_ms;
    uint32_t transition_count;
} fatfish_state_snapshot_t;

typedef struct {
    fatfish_state_snapshot_t snapshot;
} fatfish_state_machine_t;

void fatfish_state_machine_init(fatfish_state_machine_t *machine);

/** Enter the initial IDLE state. Returns true only when state changed. */
bool fatfish_state_machine_start(fatfish_state_machine_t *machine,
                                 uint64_t now_ms);

/** Leave the active state. Returns true only when state changed. */
bool fatfish_state_machine_stop(fatfish_state_machine_t *machine,
                                uint64_t now_ms);

void fatfish_state_machine_snapshot(
    const fatfish_state_machine_t *machine,
    fatfish_state_snapshot_t *snapshot);

const char *fatfish_state_name(fatfish_state_t state);

#ifdef __cplusplus
}
#endif
