#pragma once
#include "Player.h"
#include "GomokuRuleSet.h" // Need specific rules for forbidden check

class AIPlayer : public Player {
public:
    std::string name() const override { return "AI"; }
    Action getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) override;

private:
    int evaluatePos(const Board& board, Pos p, Side mySide, const GomokuRuleSet* gomokuRules) const;
};
