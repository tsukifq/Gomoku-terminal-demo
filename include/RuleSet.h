#pragma once
#include "Common.h"
#include "Board.h"
#include <string>
#include <vector>

class RuleSet {
public:
    virtual ~RuleSet() = default;

    virtual std::string name() const = 0;
    virtual int boardSize() const = 0;

    // Initialize game state (e.g. Black plays Tengen)
    virtual void initGame(GameContext& ctx, Board& board) const = 0;

    // Check if an action is valid (basic rules like bounds, empty cell)
    virtual bool validateAction(const GameContext& ctx, const Board& board, Side side, const Action& action, std::string& reason) const = 0;

    // Apply the action to the board and context
    virtual void applyAction(GameContext& ctx, Board& board, Side side, const Action& action) const = 0;

    // Evaluate the game state after a move (Win, Forbidden, etc.)
    virtual Outcome evaluateAfterAction(const GameContext& ctx, const Board& board, Side side, const Action& action) const = 0;

    // Handle timeout
    virtual Outcome onTimeout(GameContext& ctx, Side side) const = 0;
};
