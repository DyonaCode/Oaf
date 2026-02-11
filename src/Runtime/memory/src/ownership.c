#include <stddef.h>
#include "ownership.h"

void oaf_ownership_init(OafOwnershipToken* token, uint64_t id, uint32_t lifetime_depth)
{
    if (token == NULL)
    {
        return;
    }

    token->id = id;
    token->lifetime_depth = lifetime_depth;
    token->immutable_borrows = 0;
    token->mutable_borrow = false;
    token->state = OAF_OWNERSHIP_OWNED;
}

bool oaf_ownership_move(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->state != OAF_OWNERSHIP_OWNED)
    {
        return false;
    }

    if (token->immutable_borrows > 0 || token->mutable_borrow)
    {
        return false;
    }

    token->state = OAF_OWNERSHIP_MOVED;
    return true;
}

bool oaf_ownership_reacquire(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->state != OAF_OWNERSHIP_MOVED)
    {
        return false;
    }

    if (token->immutable_borrows > 0 || token->mutable_borrow)
    {
        return false;
    }

    token->state = OAF_OWNERSHIP_OWNED;
    return true;
}

bool oaf_ownership_borrow_immutable(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->state != OAF_OWNERSHIP_OWNED || token->mutable_borrow)
    {
        return false;
    }

    token->immutable_borrows++;
    return true;
}

bool oaf_ownership_release_immutable(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->immutable_borrows == 0)
    {
        return false;
    }

    token->immutable_borrows--;
    return true;
}

bool oaf_ownership_borrow_mutable(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->state != OAF_OWNERSHIP_OWNED)
    {
        return false;
    }

    if (token->mutable_borrow || token->immutable_borrows > 0)
    {
        return false;
    }

    token->mutable_borrow = true;
    return true;
}

bool oaf_ownership_release_mutable(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (!token->mutable_borrow)
    {
        return false;
    }

    token->mutable_borrow = false;
    return true;
}

bool oaf_ownership_transfer(OafOwnershipToken* source, OafOwnershipToken* destination)
{
    if (source == NULL || destination == NULL)
    {
        return false;
    }

    if (source->state != OAF_OWNERSHIP_OWNED)
    {
        return false;
    }

    if (source->immutable_borrows > 0 || source->mutable_borrow)
    {
        return false;
    }

    if (destination->immutable_borrows > 0 || destination->mutable_borrow)
    {
        return false;
    }

    destination->id = source->id;
    destination->lifetime_depth = source->lifetime_depth;
    destination->state = OAF_OWNERSHIP_OWNED;
    source->state = OAF_OWNERSHIP_MOVED;
    return true;
}

bool oaf_ownership_release(OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    if (token->state != OAF_OWNERSHIP_OWNED)
    {
        return false;
    }

    if (token->immutable_borrows > 0 || token->mutable_borrow)
    {
        return false;
    }

    token->state = OAF_OWNERSHIP_RELEASED;
    return true;
}

bool oaf_ownership_can_read(const OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    return token->state == OAF_OWNERSHIP_OWNED;
}

bool oaf_ownership_can_write(const OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    return token->state == OAF_OWNERSHIP_OWNED
        && token->immutable_borrows == 0
        && !token->mutable_borrow;
}

bool oaf_ownership_is_alive(const OafOwnershipToken* token)
{
    if (token == NULL)
    {
        return false;
    }

    return token->state == OAF_OWNERSHIP_OWNED;
}
