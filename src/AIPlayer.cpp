#include "../include/AIPlayer.h"
#include <vector>
#include <algorithm>
#include <random>

Action AIPlayer::getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) {
    Action action;
    action.type = ActionType::Place;
    action.spent = std::chrono::milliseconds(100); // Fake think time

    Side mySide = ctx.toMove;
    Side oppSide = (mySide == Side::Black) ? Side::White : Side::Black;
    
    const GomokuRuleSet* gomokuRules = dynamic_cast<const GomokuRuleSet*>(&rules);

    std::vector<Pos> moves;
    // Generate all valid moves
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            if (board.isEmpty(p)) {
                moves.push_back(p);
            }
        }
    }

    Pos bestMove = {-1, -1};
    int bestScore = -100000000;

    // Simple heuristic
    for (const auto& p : moves) {
        int score = 0;
        
        // 1. Check if I win immediately
        // Temporarily place
        // We can't easily modify board here as it is const.
        // But we can use board.countConsecutive logic.
        
        // Check my potential lines
        bool myFive = false;
        int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
        for (auto& d : dirs) {
            int count = 1 + board.countConsecutive(p, d[0], d[1], mySide) + board.countConsecutive(p, -d[0], -d[1], mySide);
            if (count >= 5) {
                if (mySide == Side::Black && count > 5) {
                    // Overline is forbidden for Black unless it also makes 5? No, Overline is strictly >5.
                    // Wait, "Five Priority": if I make 5 AND forbidden, 5 wins.
                    // If I make 6, is it 5? No, it's 6.
                    // Black Overline is forbidden.
                } else {
                    myFive = true;
                }
            }
        }

        if (myFive) {
            score = 10000000; // Win!
        } else {
            // Check Forbidden (only for Black)
            if (mySide == Side::Black && gomokuRules) {
                std::string reason;
                // We need a non-const board to test forbidden? 
                // GomokuRuleSet::isForbidden takes const Board&.
                // But it checks the board AS IS. It assumes the stone is NOT there yet?
                // No, my implementation of isForbidden checks patterns around p.
                // Wait, my isForbidden implementation calls `checkOverline` which calls `countConsecutive`.
                // `countConsecutive` counts EXISTING stones.
                // So `isForbidden` needs the stone to be ON the board?
                // Let's check `GomokuRuleSet.cpp`.
                // `checkOverline`: `count += board.countConsecutive...`
                // It assumes `p` is the center. `countConsecutive` does NOT include `p`.
                // So `count` starts at 1.
                // So `isForbidden` works for a HYPOTHETICAL move at `p` assuming `p` is currently EMPTY?
                // Yes, `countConsecutive` stops at empty.
                // So if `p` is empty, `countConsecutive` returns 0?
                // Wait. `countConsecutive` checks `get({r,c}) == side`.
                // If `p` is empty, we are checking neighbors.
                // So `isForbidden` logic in `GomokuRuleSet` assumes `p` is the stone being placed.
                // Yes.
                
                if (gomokuRules->isForbidden(board, p, reason)) {
                    score = -1000000; // Forbidden
                }
            }
        }

        if (score > -100000) { // If not forbidden
            // 2. Check if opponent wins next (Block)
            bool oppFive = false;
            for (auto& d : dirs) {
                int count = 1 + board.countConsecutive(p, d[0], d[1], oppSide) + board.countConsecutive(p, -d[0], -d[1], oppSide);
                if (count >= 5) {
                     // If opponent is Black, they might be forbidden to play here?
                     // But if they make 5, they win.
                     // If they make 6 (Overline), it's forbidden for them.
                     if (oppSide == Side::Black && count > 5) {
                         // They can't play here to win.
                     } else {
                         oppFive = true;
                     }
                }
            }
            if (oppFive) score += 5000000; // Must block!

            // 3. Positional score
            // Center is (7,7).
            int dist = std::abs(p.r - 7) + std::abs(p.c - 7);
            score += (30 - dist); // Prefer center

            // 4. Create Open Threes/Fours
            // (Simplified: just count consecutive)
             for (auto& d : dirs) {
                int myCount = 1 + board.countConsecutive(p, d[0], d[1], mySide) + board.countConsecutive(p, -d[0], -d[1], mySide);
                if (myCount == 4) score += 10000;
                if (myCount == 3) score += 1000;
                
                int oppCount = 1 + board.countConsecutive(p, d[0], d[1], oppSide) + board.countConsecutive(p, -d[0], -d[1], oppSide);
                if (oppCount == 4) score += 8000; // Block 4
                if (oppCount == 3) score += 800; // Block 3
            }
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = p;
        }
    }

    action.pos = bestMove;
    return action;
}
