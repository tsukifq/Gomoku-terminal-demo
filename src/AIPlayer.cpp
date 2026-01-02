#include "../include/AIPlayer.h"
#include <vector>
#include <algorithm>
#include <random>
#include <limits>

// Minimax with Alpha-Beta Pruning
// Depth limit controlled by difficulty
// Easy: Depth 1 (Greedy)
// Medium: Depth 2
// Hard: Depth 4 (Slow but smart)

long long evaluateBoard(const Board& board, Side mySide, Side oppSide) {
    long long score = 0;
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    // Scan entire board (inefficient but simple)
    // Better: Scan only lines with stones.
    // For now, let's just use a simplified evaluation:
    // Iterate all empty spots? No, that's for move generation.
    // Iterate all stones? Yes.
    
    // Actually, let's stick to the previous heuristic logic but apply it to the leaf node state.
    // But evaluating the whole board from scratch is slow.
    // Let's just evaluate "potential" of the board for mySide vs oppSide.
    
    auto evaluatePos = [&](Pos p, Side side) -> long long {
        long long s = 0;
        for (auto& d : dirs) {
            int count = 1;
            int r, c;
            // Forward
            r = p.r + d[0]; c = p.c + d[1];
            while (board.isValid({r, c}) && board.get({r, c}) == side) { count++; r += d[0]; c += d[1]; }
            bool open1 = board.isValid({r, c}) && board.isEmpty({r, c});
            // Backward
            r = p.r - d[0]; c = p.c - d[1];
            while (board.isValid({r, c}) && board.get({r, c}) == side) { count++; r -= d[0]; c -= d[1]; }
            bool open2 = board.isValid({r, c}) && board.isEmpty({r, c});
            
            if (count >= 5) s += 100000000;
            else if (count == 4) {
                if (open1 && open2) s += 1000000;
                else if (open1 || open2) s += 100000;
            }
            else if (count == 3) {
                if (open1 && open2) s += 100000;
                else if (open1 || open2) s += 1000;
            }
            else if (count == 2) {
                if (open1 && open2) s += 100;
                else if (open1 || open2) s += 10;
            }
        }
        return s;
    };

    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            Side s = board.get(p);
            if (s == mySide) score += evaluatePos(p, mySide);
            else if (s == oppSide) score -= evaluatePos(p, oppSide);
        }
    }
    return score;
}

// Get candidate moves (sorted by proximity to center/stones)
std::vector<Pos> getCandidates(const Board& board) {
    std::vector<Pos> moves;
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            if (board.isEmpty(p)) {
                // Optimization: Only consider moves within 2 steps of existing stones
                bool neighbor = false;
                if (board.get({7,7}) == Side::None && r==7 && c==7) { moves.push_back(p); continue; } // Center always candidate

                for (int dr = -2; dr <= 2; ++dr) {
                    for (int dc = -2; dc <= 2; ++dc) {
                        if (dr==0 && dc==0) continue;
                        Pos n = {r+dr, c+dc};
                        if (board.isValid(n) && !board.isEmpty(n)) {
                            neighbor = true;
                            break;
                        }
                    }
                    if (neighbor) break;
                }
                if (neighbor) moves.push_back(p);
            }
        }
    }
    return moves;
}

long long minimax(Board& board, int depth, long long alpha, long long beta, bool maximizingPlayer, Side mySide, Side oppSide, const GomokuRuleSet* rules) {
    if (depth == 0) {
        return evaluateBoard(board, mySide, oppSide);
    }

    std::vector<Pos> moves = getCandidates(board);
    if (moves.empty()) return 0;

    if (maximizingPlayer) {
        long long maxEval = -std::numeric_limits<long long>::max();
        for (const auto& p : moves) {
            // Forbidden check for Black
            if (mySide == Side::Black && rules) {
                std::string reason;
                // Note: isForbidden checks if placing at p creates forbidden pattern.
                // We need to temporarily place it? No, isForbidden assumes p is the move.
                // But isForbidden uses board state.
                // My implementation of isForbidden assumes p is NOT on board yet?
                // Let's check GomokuRuleSet.cpp again.
                // It calls checkOverline -> countConsecutive.
                // countConsecutive counts EXISTING stones.
                // So if we haven't placed stone yet, countConsecutive returns 0 for p.
                // So isForbidden logic needs to be careful.
                // Actually, for simplicity in Minimax, let's skip forbidden check deep in tree or assume simple heuristic.
                // Or just check it:
                if (rules->isForbidden(board, p, reason)) continue; 
            }

            board.set(p, mySide);
            long long eval = minimax(board, depth - 1, alpha, beta, false, mySide, oppSide, rules);
            board.clear(p); // Backtrack
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        long long minEval = std::numeric_limits<long long>::max();
        for (const auto& p : moves) {
            // Forbidden check for Opponent (if Black)
            if (oppSide == Side::Black && rules) {
                std::string reason;
                if (rules->isForbidden(board, p, reason)) continue;
            }

            board.set(p, oppSide);
            long long eval = minimax(board, depth - 1, alpha, beta, true, mySide, oppSide, rules);
            board.clear(p); // Backtrack
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

Action AIPlayer::getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) {
    Action action;
    action.type = ActionType::Place;
    action.spent = std::chrono::milliseconds(100); 

    Side mySide = ctx.toMove;
    Side oppSide = (mySide == Side::Black) ? Side::White : Side::Black;
    const GomokuRuleSet* gomokuRules = dynamic_cast<const GomokuRuleSet*>(&rules);

    // Clone board for simulation
    Board simBoard = board; 

    int maxDepth = 2;
    if (difficulty == 1) maxDepth = 1;
    else if (difficulty == 3) maxDepth = 4;

    std::vector<Pos> moves = getCandidates(simBoard);
    
    Pos bestMove = {-1, -1};
    long long bestScore = -std::numeric_limits<long long>::max();

    // Root level search
    // Time limit check could be added here
    auto startTime = std::chrono::steady_clock::now();
    int timeLimitMs = 15000; // 15s limit
    if (difficulty == 1) timeLimitMs = 1000;
    if (difficulty == 2) timeLimitMs = 5000;

    for (const auto& p : moves) {
        // Check time
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() > timeLimitMs) {
            if (bestMove.r == -1) bestMove = p; // Ensure we have a move
            break; 
        }

        // Rule: White must play on their own side (row >= 7) on first move
        if (mySide == Side::White && ctx.turnIndex == 1) {
            if (p.r < 7) continue;
        }
        
        // Forbidden check
        if (mySide == Side::Black && gomokuRules) {
            std::string reason;
            if (gomokuRules->isForbidden(simBoard, p, reason)) continue;
        }

        simBoard.set(p, mySide);
        long long score = minimax(simBoard, maxDepth - 1, -std::numeric_limits<long long>::max(), std::numeric_limits<long long>::max(), false, mySide, oppSide, gomokuRules);
        simBoard.clear(p);

        if (score > bestScore) {
            bestScore = score;
            bestMove = p;
        }
    }

    action.pos = bestMove;
    return action;
}
