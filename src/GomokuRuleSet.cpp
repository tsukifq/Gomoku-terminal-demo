#include "../include/GomokuRuleSet.h"
#include <algorithm>
#include <iostream>

void GomokuRuleSet::initGame(GameContext& ctx, Board& board) const {
    ctx.toMove = Side::Black;
    ctx.turnIndex = 0;
    ctx.phase = Phase::Opening;
    ctx.blackTimeoutWarnings = 0;
    ctx.whiteTimeoutWarnings = 0;
    ctx.pendingForbidden = false;
    
    // Black starts at Tengen (7, 7)
    Pos center = {7, 7};
    board.set(center, Side::Black);
    ctx.lastAction = Action{ActionType::Place, center, std::chrono::milliseconds(0)};
    ctx.turnIndex = 1;
    ctx.toMove = Side::White;
}

bool GomokuRuleSet::validateAction(const GameContext& ctx, const Board& board, Side side, const Action& action, std::string& reason) const {
    if (action.type == ActionType::Resign || action.type == ActionType::OfferDraw || 
        action.type == ActionType::AcceptDraw || action.type == ActionType::RejectDraw) {
        return true;
    }

    if (action.type == ActionType::ClaimForbidden) {
        if (ctx.phase != Phase::PendingClaim) {
            reason = "Cannot claim forbidden move now.";
            return false;
        }
        if (side == Side::Black) {
            reason = "Black cannot claim their own forbidden move.";
            return false;
        }
        return true;
    }

    if (action.type == ActionType::Place) {
        if (!action.pos.has_value()) {
            reason = "No position specified.";
            return false;
        }
        Pos p = action.pos.value();
        if (!board.isValid(p)) {
            reason = "Position out of bounds.";
            return false;
        }
        if (!board.isEmpty(p)) {
            reason = "Position already occupied.";
            return false;
        }

        // 根据规则，白棋第一手应在天元为界自己一侧布子
        if (side == Side::White && ctx.turnIndex == 1) {
            if (p.r < 7) {
                reason = "Rule: White must play on your own side of the board(row >= 7) on first move.";
                return false;
            }
        }
        return true;
    }

    reason = "Unknown action type.";
    return false;
}

void GomokuRuleSet::applyAction(GameContext& ctx, Board& board, Side side, const Action& action) const {
    if (action.type == ActionType::Place) {
        // If we were in PendingClaim and White played a move (Place), the claim is waived.
        if (ctx.phase == Phase::PendingClaim) {
            ctx.phase = Phase::Normal;
            ctx.pendingForbidden = false;
        }

        board.set(action.pos.value(), side);
        ctx.lastAction = action;
        ctx.turnIndex++;
        ctx.toMove = (side == Side::Black) ? Side::White : Side::Black;
        
        // Phase update handled in evaluateAfterAction usually, but here we just update state.
        if (ctx.phase == Phase::Opening && ctx.turnIndex > 2) {
            ctx.phase = Phase::Normal;
        }
    }
}

Outcome GomokuRuleSet::evaluateAfterAction(const GameContext& ctx, const Board& board, Side side, const Action& action) const {
    Outcome outcome;
    outcome.status = GameStatus::Ongoing;

    if (action.type == ActionType::Resign) {
        outcome.status = GameStatus::Win;
        outcome.winner = (side == Side::Black) ? Side::White : Side::Black;
        outcome.reason = "Resignation";
        return outcome;
    }
    
    if (action.type == ActionType::ClaimForbidden) {
        // 举手！
        outcome.status = GameStatus::Forbidden;
        outcome.winner = Side::White;
        outcome.reason = "Forbidden move claimed";
        return outcome;
    }

    if (action.type != ActionType::Place) return outcome;

    Pos p = action.pos.value();

    // 4 directions
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    bool isFive = false;
    bool isOverline = false;

    for (auto& d : dirs) {
        int count = 1;
        count += board.countConsecutive(p, d[0], d[1], side);
        count += board.countConsecutive(p, -d[0], -d[1], side);
        
        if (count == 5) isFive = true;
        if (count > 5) isOverline = true;
    }

    if (side == Side::White) {
        if (isFive || isOverline) {
            outcome.status = GameStatus::Win;
            outcome.winner = Side::White;
            outcome.reason = isOverline ? "White Long Chain (Win)" : "White Five";
            return outcome;
        }
    } else {
        if (isFive) {
            // Five Priority: Even if forbidden, 5 wins.
            outcome.status = GameStatus::Win;
            outcome.winner = Side::Black;
            outcome.reason = "Black Five";
            return outcome;
        }
        
        // 禁手check逻辑
        std::string forbiddenReason;
        if (isForbidden(board, p, forbiddenReason)) {
            // a status for PendingClaim.
            outcome.status = GameStatus::PendingClaim;
            outcome.reason = forbiddenReason;
            return outcome;
        }
    }

    if (board.isFull()) {
        outcome.status = GameStatus::Draw;
        outcome.reason = "Board Full";
    }

    return outcome;
}

Outcome GomokuRuleSet::onTimeout(GameContext& ctx, Side side) const {
    Outcome outcome;
    outcome.status = GameStatus::Ongoing;
    
    int& warnings = (side == Side::Black) ? ctx.blackTimeoutWarnings : ctx.whiteTimeoutWarnings;
    warnings++;
    
    if (warnings > 3) {
        outcome.status = GameStatus::TimeoutLose;
        outcome.winner = (side == Side::Black) ? Side::White : Side::Black;
        outcome.reason = "Timeout (Max Warnings)";
    } else {
        outcome.reason = "Timeout Warning " + std::to_string(warnings);
    }
    return outcome;
}

// --- Forbidden Logic ---
bool GomokuRuleSet::isForbidden(const Board& board, Pos p, std::string& reason) const {
    // 1. 长连
    if (checkOverline(board, p)) {
        reason = "Overline (6+)";
        return true;
    }
    
    // 2. 三三
    if (checkThreeThree(board, p)) {
        reason = "Three-Three";
        return true;
    }
    
    // 3. 四四
    if (checkFourFour(board, p)) {
        reason = "Four-Four";
        return true;
    }
    
    return false;
}

bool GomokuRuleSet::checkOverline(const Board& board, Pos p) const {
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for (auto& d : dirs) {
        int count = 1;
        count += board.countConsecutive(p, d[0], d[1], Side::Black);
        count += board.countConsecutive(p, -d[0], -d[1], Side::Black);
        if (count > 5) return true;
    }
    return false;
}

std::vector<int> getLinePattern(const Board& board, Pos p, int dr, int dc) {
    std::vector<int> line;
    // Look back 4
    for (int i = -4; i <= 4; ++i) {
        Pos curr = {p.r + i * dr, p.c + i * dc};
        if (!board.isValid(curr)) {
            line.push_back(2);
        } else {
            Side s = board.get(curr);
            if (s == Side::Black) line.push_back(1);
            else if (s == Side::White) line.push_back(2);
            else line.push_back(0);
        }
    }
    return line;
}

bool GomokuRuleSet::checkThreeThree(const Board& board, Pos p) const {
    int openThreeCount = 0;
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (auto& d : dirs) {
        if (isOpenThree(board, p, d[0], d[1])) {
            openThreeCount++;
        }
    }
    return openThreeCount >= 2;
}

bool GomokuRuleSet::checkFourFour(const Board& board, Pos p) const {
    int fourCount = 0;
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (auto& d : dirs) {
        if (isFour(board, p, d[0], d[1])) {
            fourCount++;
        }
    }
    return fourCount >= 2;
}

// Check if placing at p creates an Open Three in direction (dr, dc)
// An Open Three must be able to form a straight 4.
// Patterns (1=Self, 0=Empty):
// 01110 (Standard)
// 010110 (Broken)
// 011010 (Broken)
bool GomokuRuleSet::isOpenThree(const Board& board, Pos p, int dr, int dc) const {
    // We need to check the line.
    // Center is at index 4 in the pattern vector.
    std::vector<int> line = getLinePattern(board, p, dr, dc);
    
    // We are looking for specific patterns centered at 4 (where we just played '1')
    // The stone at 4 is '1'.
    
    // Patterns for Open Three (must be unblocked on both sides)
    // "01110" -> indices 3,4,5 are 1. 2 and 6 are 0.
    // "011010" -> indices 3,4,6 are 1. 5 is 0. 2 and 7 are 0.
    // "010110" -> indices 2,4,5 are 1. 3 is 0. 1 and 6 are 0.
    
    // Note: The pattern vector has size 9. Indices 0..8. Center is 4.
    
    // 1. Straight: .XXX.
    // Check if we have 111 centered at 4.
    // Could be 111 at 3,4,5 -> check 2=0, 6=0.
    // Could be 111 at 4,5,6 -> check 3=0, 7=0. (Wait, p is the new stone)
    // p is ALWAYS at 4.
    
    // Case A: 111 (p is middle or end)
    // If p is middle: 1(p)1 -> 3=1, 4=1, 5=1. Ends 2=0, 6=0.
    if (line[3]==1 && line[4]==1 && line[5]==1) {
        if (line[2]==0 && line[6]==0) return true;
    }
    // If p is left: (p)11 -> 4=1, 5=1, 6=1. Ends 3=0, 7=0.
    if (line[4]==1 && line[5]==1 && line[6]==1) {
        if (line[3]==0 && line[7]==0) return true;
    }
    // If p is right: 11(p) -> 2=1, 3=1, 4=1. Ends 1=0, 5=0.
    if (line[2]==1 && line[3]==1 && line[4]==1) {
        if (line[1]==0 && line[5]==0) return true;
    }
    
    // Case B: Broken 1011 or 1101
    // 1(p)01 -> 4=1, 5=0, 6=1, 7=1. Ends 3=0, 8=0.
    if (line[4]==1 && line[5]==0 && line[6]==1 && line[7]==1) {
        if (line[3]==0 && line[8]==0) return true;
    }
    // 10(p)1 -> 3=1, 4=0 (impossible, p is 1), ...
    // p is 1.
    // 1011 where p is the single 1: (p)011 -> 4=1, 5=0, 6=1, 7=1. (Same as above)
    // 1011 where p is in the 11: 10(p)1 -> 2=1, 3=0, 4=1, 5=1. Ends 1=0, 6=0.
    if (line[2]==1 && line[3]==0 && line[4]==1 && line[5]==1) {
        if (line[1]==0 && line[6]==0) return true;
    }
    // 1011 where p is end: 101(p) -> 1=1, 2=0, 3=1, 4=1. Ends 0=0, 5=0.
    if (line[1]==1 && line[2]==0 && line[3]==1 && line[4]==1) {
        if (line[0]==0 && line[5]==0) return true;
    }
    
    // Symmetric for 1101
    // 110(p) -> 2=1, 3=1, 4=0 (impossible)
    // (p)101 -> 4=1, 5=1, 6=0, 7=1. Ends 3=0, 8=0.
    if (line[4]==1 && line[5]==1 && line[6]==0 && line[7]==1) {
        if (line[3]==0 && line[8]==0) return true;
    }
    // 1(p)01 -> 3=1, 4=1, 5=0, 6=1. Ends 2=0, 7=0.
    if (line[3]==1 && line[4]==1 && line[5]==0 && line[6]==1) {
        if (line[2]==0 && line[7]==0) return true;
    }
    
    return false;
}

// Check if placing at p creates a Four (Straight 4 or Broken 4 that can become 5)
// Patterns: 1111, 10111, 11011, 11101
// Must NOT be Overline (6+).
// Must be able to become 5.
bool GomokuRuleSet::isFour(const Board& board, Pos p, int dr, int dc) const {
    std::vector<int> line = getLinePattern(board, p, dr, dc);
    
    // We need to find a group of 4 stones (including p at 4) that has at least one empty spot next to it to make 5.
    // Actually, "Four" definition:
    // "Straight Four": 011110 (Live Four) -> creates two fives.
    // "Four": 011112 or 211110 (Rush Four) -> creates one five.
    // "Broken Four": 10111, 11011, 11101 -> creates one five.
    
    // Strategy: Check all substrings of length 5 that include index 4.
    // If a substring has four 1s and one 0, it's a potential Four.
    // We must ensure that filling that 0 creates a 5.
    
    // Substrings of length 5 containing index 4:
    // Start indices: 0, 1, 2, 3, 4. (End indices: 4, 5, 6, 7, 8)
    
    int countFours = 0;
    
    for (int start = 0; start <= 4; ++start) {
        int end = start + 4; // length 5
        if (end > 8) continue;
        
        // Check this window of 5
        int ones = 0;
        int zeros = 0;
        int zeroIdx = -1;
        
        for (int k = start; k <= end; ++k) {
            if (line[k] == 1) ones++;
            else if (line[k] == 0) {
                zeros++;
                zeroIdx = k;
            } else {
                // Blocked by 2
            }
        }
        
        if (ones == 4 && zeros == 1) {
            // Found a pattern like 11101.
            // Does filling the 0 make a 5?
            // We need to check if the 0 is adjacent to the 4 ones in a way that makes 5.
            // Actually, if we have 4 ones and 1 zero in a window of 5, filling the zero ALWAYS makes 5.
            // The only catch is if it makes 6 (Overline).
            // But we are checking "isFour", not "isFive".
            // The definition of Four is "can become Five".
            // So yes, this is a Four.
            return true;
        }
    }
    
    // Also check Straight Four (Live Four) 011110 separately?
    // The above logic covers 01111 (with one 0).
    // Wait, 01111 is 4 ones and 1 zero. Yes.
    // But 1111 is length 4.
    // If we have 1111 (compact), we need to check ends.
    // 1111 at 4,5,6,7 -> check 3 or 8 is 0.
    // My window scan above covers "11110" and "01111".
    
    return false;
}
