/*
 * BOSPECTRA V3 — Watchdog Rules Subsystem
 * kernel/media/bospectra/watchdog/watchdog_rules.h
 */

#ifndef BOSPECTRA_V3_WATCHDOG_RULES_H
#define BOSPECTRA_V3_WATCHDOG_RULES_H

#include "../playback/session/playback_session.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RULE_RESULT_OK = 0,
    RULE_RESULT_DECODER_TIMEOUT,
    RULE_RESULT_RENDER_TIMEOUT,
    RULE_RESULT_PACKET_STALL,
    RULE_RESULT_FRAME_STALL,
    RULE_RESULT_BLACK_SCREEN
} BOSPECTRA_WatchdogRuleResult;

void                         bospectra_watchdog_rules_init(void);
void                         bospectra_watchdog_rules_shutdown(void);

BOSPECTRA_WatchdogRuleResult bospectra_watchdog_evaluate_rules(const PlaybackSessionCtx* sess);

#endif /* BOSPECTRA_V3_WATCHDOG_RULES_H */
