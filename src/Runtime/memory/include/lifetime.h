#ifndef OAFLANG_LIFETIME_H
#define OAFLANG_LIFETIME_H

#include <stdbool.h>
#include <stdint.h>
#include "ownership.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafLifetimeTracker
{
    uint32_t current_depth;
    uint32_t max_depth;
} OafLifetimeTracker;

void oaf_lifetime_init(OafLifetimeTracker* tracker, uint32_t max_depth);
bool oaf_lifetime_enter_scope(OafLifetimeTracker* tracker);
bool oaf_lifetime_exit_scope(OafLifetimeTracker* tracker);
void oaf_lifetime_bind_token(OafLifetimeTracker* tracker, OafOwnershipToken* token);
bool oaf_lifetime_token_is_valid(const OafLifetimeTracker* tracker, const OafOwnershipToken* token);
bool oaf_lifetime_token_can_access(const OafLifetimeTracker* tracker, const OafOwnershipToken* token);

#ifdef __cplusplus
}
#endif

#endif
