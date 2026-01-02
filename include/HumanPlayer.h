#pragma once
#include "Player.h"

class HumanPlayer : public Player {
public:
    std::string name() const override { return "Human"; }
    Action getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) override;
};
