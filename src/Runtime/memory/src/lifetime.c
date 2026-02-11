#include <stddef.h>
#include "lifetime.h"

void oaf_lifetime_init(OafLifetimeTracker* tracker, uint32_t max_depth)
{
    if (tracker == NULL)
    {
        return;
    }

    tracker->current_depth = 0;
    tracker->max_depth = max_depth;
}

bool oaf_lifetime_enter_scope(OafLifetimeTracker* tracker)
{
    if (tracker == NULL)
    {
        return false;
    }

    if (tracker->current_depth >= tracker->max_depth)
    {
        return false;
    }

    tracker->current_depth++;
    return true;
}

bool oaf_lifetime_exit_scope(OafLifetimeTracker* tracker)
{
    if (tracker == NULL)
    {
        return false;
    }

    if (tracker->current_depth == 0)
    {
        return false;
    }

    tracker->current_depth--;
    return true;
}

void oaf_lifetime_bind_token(OafLifetimeTracker* tracker, OafOwnershipToken* token)
{
    if (tracker == NULL || token == NULL)
    {
        return;
    }

    token->lifetime_depth = tracker->current_depth;
}

bool oaf_lifetime_token_is_valid(const OafLifetimeTracker* tracker, const OafOwnershipToken* token)
{
    if (tracker == NULL || token == NULL)
    {
        return false;
    }

    return token->lifetime_depth <= tracker->current_depth;
}

bool oaf_lifetime_token_can_access(const OafLifetimeTracker* tracker, const OafOwnershipToken* token)
{
    return oaf_lifetime_token_is_valid(tracker, token) && oaf_ownership_can_read(token);
}
