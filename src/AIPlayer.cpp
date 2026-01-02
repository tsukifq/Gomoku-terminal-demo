#include "../include/AIPlayer.h"
#include <vector>
#include <algorithm>
#include <random>
#include <limits>

// 带Alpha-Beta剪枝的极大极小算法
// 搜索深度由难度控制
// 简单: 深度 1 (贪婪)
// 中等: 深度 2
// 困难: 深度 4 (聪明)

long long evaluateBoard(const Board& board, Side mySide, Side oppSide) {
    long long score = 0;
    int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    // 扫描整个棋盘（效率较低）
    // 只扫描有棋子的线
    // 使用简化的评估，只评估棋盘对 mySide 与 oppSide 的“潜力”。
    
    auto evaluatePos = [&](Pos p, Side side) -> long long {
        long long s = 0;
        for (auto& d : dirs) {
            int count = 1;
            int r, c;
            // 正向
            r = p.r + d[0]; c = p.c + d[1];
            while (board.isValid({r, c}) && board.get({r, c}) == side) { count++; r += d[0]; c += d[1]; }
            bool open1 = board.isValid({r, c}) && board.isEmpty({r, c});
            // 反向
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

// 获取候选走法（按距离中心/棋子的远近排序）
std::vector<Pos> getCandidates(const Board& board) {
    std::vector<Pos> moves;
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            if (board.isEmpty(p)) {
                // 剪枝：只考虑现有棋子周围 2 步范围内的走法
                bool neighbor = false;
                if (board.get({7,7}) == Side::None && r==7 && c==7) { moves.push_back(p); continue; } // 中心点总是候选

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
            // 黑方禁手检查
            if (mySide == Side::Black && rules) {
                std::string reason;
                // isForbidden 检查在 p 点落子是否会形成禁手模式。
                // 实际上，为了简化 Minimax，在树的深层会跳过禁手检查或假设简单的启发式。
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
            // 对手禁手检查（如果对手是黑方）
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

    // 根节点搜索
    // 可以在此处添加时间限制检查
    auto startTime = std::chrono::steady_clock::now();
    int timeLimitMs = 15000; // 15秒限制
    if (difficulty == 1) timeLimitMs = 1000;
    if (difficulty == 2) timeLimitMs = 5000;

    for (const auto& p : moves) {
        // 检查时间
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() > timeLimitMs) {
            if (bestMove.r == -1) bestMove = p;
            break; 
        }

        // 规则：白方第一手必须下在自己的半场（行 >= 7）
        if (mySide == Side::White && ctx.turnIndex == 1) {
            if (p.r < 7) continue;
        }
        
        // 禁手检查
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
