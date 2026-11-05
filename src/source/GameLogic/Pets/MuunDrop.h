#pragma once

#include "GameLogic/Pets/Muun.h"
#include <algorithm>
#include <optional>

namespace GameLogic::Pets::Muun
{
constexpr std::uint8_t MoveOperation = 9, DepositAtOperation = 10;
constexpr std::uint8_t LastOperation = DepositAtOperation;

struct DropRequest
{
    std::uint8_t operation;
    int source, target;
    int equipment = -1, type = -1;
};

bool RequestDrop(const DropRequest& request);

inline int FreeCollectionSlot(const State& state)
{
    for (int slot = 2; slot < state.capacity; ++slot)
        if (std::none_of(state.items.begin(), state.items.end(), [slot](const auto& item) { return item.slot == slot; })) return slot;
    return -1;
}

inline std::optional<DropRequest> RequestForDrop(const State& state, bool fromBag, int source, int type, int target)
{
    if (!state.enabled || state.pending || source < 0 || source > 255 || target < 0 || target >= state.capacity)
        return std::nullopt;
    const auto destination = std::find_if(state.items.begin(), state.items.end(), [target](const auto& i) { return i.slot == target; });
    const auto ruleFor = [&state](int itemType) -> const Species* {
        for (const auto& rule : state.species) if (rule.type == itemType) return &rule;
        return nullptr;
    };
    if (fromBag)
    {
        if (IsType(type) && ruleFor(type) && (target < 2 || destination == state.items.end())) {
            if (state.supportsSlotMoves) return DropRequest{DepositAtOperation, source, target};
            if (FreeCollectionSlot(state) < 0) return std::nullopt;
            return DropRequest{1, source, 0, target < 2 ? target : -1, type};
        }
        if (destination == state.items.end()) return std::nullopt;
        const auto* rule = ruleFor(destination->type);
        if (!rule) return std::nullopt;
        if (type == rule->repair && destination->life < destination->maximumLife)
            return DropRequest{7, target, source};
        if (type == rule->stone && rule->evolution >= 0 && destination->level == rule->maximumLevel)
            return DropRequest{6, target, source};
        return std::nullopt;
    }
    const auto dragged = std::find_if(state.items.begin(), state.items.end(), [source](const auto& i) { return i.slot == source; });
    if (dragged == state.items.end() || dragged->type != type || source == target) return std::nullopt;
    if (destination != state.items.end() && source >= 2 && dragged->type == destination->type && dragged->level == 0) {
        const auto* rule = ruleFor(destination->type);
        if (rule && destination->level < rule->maximumLevel) return DropRequest{5, target, source};
        return std::nullopt;
    }
    if (target < 2) return DropRequest{3, source, target};
    if (destination == state.items.end()) {
        if (state.supportsSlotMoves) return DropRequest{MoveOperation, source, target};
        if (source < 2 && FreeCollectionSlot(state) >= 0) return DropRequest{4, source, 0};
    }
    return std::nullopt;
}
}
