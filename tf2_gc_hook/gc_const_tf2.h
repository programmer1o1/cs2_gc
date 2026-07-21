#pragma once

#include <cstdint>

// TF2-specific GC constants. See docs/tf2_live_hook.md for how each of these
// was verified (either against a real items_game.txt/GCSDK source, or against
// a live TF2 client).

// CSOEconItemAttribute def_index for "attach particle effect" (drives
// Unusual hat effects). Value is stored as a float bit-pattern in the
// generic CSOEconItemAttribute.value field, same convention CS:GO uses
// for e.g. paint kit attributes (see ItemSchema::AttributeFloat).
constexpr uint32_t AttributeParticleEffectTF2 = 134;

// CSOEconItemAttribute def_index for "is australium item" (attribute_class
// "is_australium_item", stored_as_integer per the real schema -- confirmed
// in a real items_game.txt's attributes block). Real Australium weapons are
// just the base weapon's own defindex carrying this one attribute at Strange
// quality -- not a separate item, and NOT driven by CSOEconItem.style (see
// gc_client_tf2.cpp's BuildEconItem for why the style system doesn't apply
// to these defindexes).
constexpr uint32_t AttributeIsAustraliumItemTF2 = 2027;

// Shared object type (type_id field in CMsgSOCacheSubscribed_SubscribedType).
// Economy items are SO type 1 across every Valve GC game (CS:GO, TF2, Dota2).
constexpr uint32_t SOTypeItemTF2 = 1;

// CSOEconGameAccountClient SO type -- same value as csgo_gc/gc_const_csgo.h's
// SOTypeGameAccountClient, a Valve-wide constant (see docs/tf2_live_hook.md).
// Carries additional_backpack_slots; never sending this object at all left
// the real client assuming whatever default backpack capacity it falls back
// to, which a 300+ item injected backpack can exceed ("not enough space").
constexpr uint32_t SOTypeGameAccountClientTF2 = 7;

// CSOEconItem origin. csgo_gc/gc_const_csgo.h has the same named constant
// (ItemOriginBaseItem = 22) for CS:GO's equivalent "locally-injected default
// item" case; using 0 (plausibly k_EItemOriginInvalid in Valve's shared
// origin enum) here instead was a candidate reason real-defindex items still
// didn't show up in a live test.
constexpr uint32_t ItemOriginBaseItemTF2 = 22;

// CMsgAdjustItemEquippedState.item_id / .new_slot sentinels for "unequip",
// matching csgo_gc/inventory.cpp's ItemIdInvalid/SlotUneqip constants (same
// generic message/semantics -- confirmed TF2's real client sends this exact
// message via CInventoryManager::UpdateInventoryEquippedState in
// econ_item_inventory.cpp).
constexpr uint64_t ItemIdInvalidTF2 = 0;
constexpr uint32_t SlotUnequipTF2 = 0xffff;

// CMsgSOCacheSubscribed/CMsgSOMultipleObjects.version -- csgo_gc/inventory.cpp
// sets this same fixed nonzero constant (InventoryVersion) on every SO cache
// message it sends (BuildCacheSubscription, AddToMultipleObjects,
// ToSingleObject). We never set it at all here, which is a likely reason
// equip updates (CMsgSOMultipleObjects) got silently ignored client-side
// even though the message demonstrably arrived (no parse/validation
// errors): GCSDK's shared object cache tracks a version per cache and may
// drop updates that don't look newer than what it already has, and an
// entirely absent/zero version is the least "new" a message can look.
constexpr uint64_t InventoryVersionTF2 = 7523377975160828514;
