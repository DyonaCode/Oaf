#ifndef OAFLANG_OWNERSHIP_H
#define OAFLANG_OWNERSHIP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OafOwnershipState
{
    OAF_OWNERSHIP_UNINITIALIZED = 0,
    OAF_OWNERSHIP_OWNED = 1,
    OAF_OWNERSHIP_MOVED = 2,
    OAF_OWNERSHIP_RELEASED = 3
} OafOwnershipState;

typedef struct OafOwnershipToken
{
    uint64_t id;
    uint32_t lifetime_depth;
    uint32_t immutable_borrows;
    bool mutable_borrow;
    OafOwnershipState state;
} OafOwnershipToken;

void oaf_ownership_init(OafOwnershipToken* token, uint64_t id, uint32_t lifetime_depth);
bool oaf_ownership_move(OafOwnershipToken* token);
bool oaf_ownership_reacquire(OafOwnershipToken* token);
bool oaf_ownership_borrow_immutable(OafOwnershipToken* token);
bool oaf_ownership_release_immutable(OafOwnershipToken* token);
bool oaf_ownership_borrow_mutable(OafOwnershipToken* token);
bool oaf_ownership_release_mutable(OafOwnershipToken* token);
bool oaf_ownership_transfer(OafOwnershipToken* source, OafOwnershipToken* destination);
bool oaf_ownership_release(OafOwnershipToken* token);
bool oaf_ownership_can_read(const OafOwnershipToken* token);
bool oaf_ownership_can_write(const OafOwnershipToken* token);
bool oaf_ownership_is_alive(const OafOwnershipToken* token);

#ifdef __cplusplus
}
#endif

#endif
