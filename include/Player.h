#pragma once
#include "Common.h"
#include "Board.h"
#include "RuleSet.h"

class Player {
public:
    virtual ~Player() = default;
    virtual std::string name() const = 0;
    virtual Action getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) = 0;
};
