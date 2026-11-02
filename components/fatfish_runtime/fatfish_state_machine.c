/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fatfish_state_machine.h"

#include <string.h>

static bool transition(fatfish_state_machine_t *machine,
                       fatfish_state_t next, uint64_t now_ms)
{
    if (!machine || machine->snapshot.current == next) {
        return false;
    }

    machine->snapshot.previous = machine->snapshot.current;
    machine->snapshot.current = next;
    machine->snapshot.entered_at_ms = now_ms;
    machine->snapshot.transition_count++;
    return true;
}

void fatfish_state_machine_init(fatfish_state_machine_t *machine)
{
    if (!machine) {
        return;
    }

    memset(machine, 0, sizeof(*machine));
    machine->snapshot.current = FATFISH_STATE_STOPPED;
    machine->snapshot.previous = FATFISH_STATE_STOPPED;
}

bool fatfish_state_machine_start(fatfish_state_machine_t *machine,
                                 uint64_t now_ms)
{
    return transition(machine, FATFISH_STATE_IDLE, now_ms);
}

bool fatfish_state_machine_stop(fatfish_state_machine_t *machine,
                                uint64_t now_ms)
{
    return transition(machine, FATFISH_STATE_STOPPED, now_ms);
}

void fatfish_state_machine_snapshot(
    const fatfish_state_machine_t *machine,
    fatfish_state_snapshot_t *snapshot)
{
    if (machine && snapshot) {
        *snapshot = machine->snapshot;
    }
}

const char *fatfish_state_name(fatfish_state_t state)
{
    switch (state) {
    case FATFISH_STATE_STOPPED:
        return "stopped";
    case FATFISH_STATE_IDLE:
        return "idle";
    default:
        return "unknown";
    }
}
