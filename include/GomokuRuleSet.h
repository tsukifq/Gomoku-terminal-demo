#pragma once
#include "RuleSet.h"
#include <vector>

class GomokuRuleSet : public RuleSet {
public:
    std::string name() const override { return "Gomoku (Renju-like)"; }
    int boardSize() const override { return 15; }

    void initGame(GameContext& ctx, Board& board) const override;
    bool validateAction(const GameContext& ctx, const Board& board, Side side, const Action& action, std::string& reason) const override;
    void applyAction(GameContext& ctx, Board& board, Side side, const Action& action) const override;
    Outcome evaluateAfterAction(const GameContext& ctx, const Board& board, Side side, const Action& action) const override;
    Outcome onTimeout(GameContext& ctx, Side side) const override;

    // Public for testing/AI
    bool isForbidden(const Board& board, Pos p, std::string& reason) const;

private:
    // Forbidden logic helpers
    bool checkOverline(const Board& board, Pos p) const;
    bool checkThreeThree(const Board& board, Pos p) const;
    bool checkFourFour(const Board& board, Pos p) const;
    
    // Helper to analyze lines
    // Returns true if a pattern is found
    bool isOpenThree(const Board& board, Pos p, int dr, int dc) const;
    bool isFour(const Board& board, Pos p, int dr, int dc) const; // Straight 4 or broken 4 that can become 5
    bool isFive(const Board& board, Pos p, int dr, int dc) const;
};
