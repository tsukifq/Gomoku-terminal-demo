#include "../include/AIPlayer.h"
#include <vector>
#include <algorithm>
#include <random>

Action AIPlayer::getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) {
    Action action;
    action.type = ActionType::Place;
    action.spent = std::chrono::milliseconds(100); // 模拟思考时间

    Side mySide = ctx.toMove;
    Side oppSide = (mySide == Side::Black) ? Side::White : Side::Black;
    
    const GomokuRuleSet* gomokuRules = dynamic_cast<const GomokuRuleSet*>(&rules);

    std::vector<Pos> moves;
    // 生成所有合法走法
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            if (board.isEmpty(p)) {
                moves.push_back(p);
            }
        }
    }

    Pos bestMove = {-1, -1};
    long long bestScore = -1000000000;

    // 简单启发式评估
    auto evaluateLine = [&](Pos p, int dr, int dc, Side side) -> int {
        int count = 1;
        int r, c;
        
        // 向前搜索
        r = p.r + dr; c = p.c + dc;
        while (board.isValid({r, c}) && board.get({r, c}) == side) {
            count++; r += dr; c += dc;
        }
        bool open1 = board.isValid({r, c}) && board.isEmpty({r, c});
        
        // 向后搜索
        r = p.r - dr; c = p.c - dc;
        while (board.isValid({r, c}) && board.get({r, c}) == side) {
            count++; r -= dr; c -= dc;
        }
        bool open2 = board.isValid({r, c}) && board.isEmpty({r, c});
        
        if (count >= 5) return 100000000;
        if (count == 4) {
            if (open1 && open2) return 1000000; // 活四
            if (open1 || open2) return 100000;  // 冲四
        }
        if (count == 3) {
            if (open1 && open2) return 100000;  // 活三
            if (open1 || open2) return 1000;    // 眠三
        }
        if (count == 2) {
            if (open1 && open2) return 100;     // 活二
            if (open1 || open2) return 10;      // 眠二
        }
        return 0;
    };

    for (const auto& p : moves) {
        // 规则：白棋第一手必须下在己方半场（行号 >= 7）
        if (mySide == Side::White && ctx.turnIndex == 1) {
            if (p.r < 7) continue;
        }

        long long score = 0;
        long long myScore = 0;
        long long oppScore = 0;
        
        int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

        // 1. 评估己方进攻
        for (auto& d : dirs) {
            myScore += evaluateLine(p, d[0], d[1], mySide);
        }

        // 2. 检查禁手（仅在不能立即获胜时检查）
        // 五连优先：如果成五，无论是否禁手都算赢。
        bool isWin = (myScore >= 100000000);
        if (!isWin && mySide == Side::Black && gomokuRules) {
            std::string reason;
            if (gomokuRules->isForbidden(board, p, reason)) {
                continue; // 跳过禁手
            }
        }

        // 3. 评估对手进攻（防守）
        for (auto& d : dirs) {
            oppScore += evaluateLine(p, d[0], d[1], oppSide);
        }

        // 综合评分。己方获胜的优先级略高于防守。
        score = myScore + (long long)(oppScore * 0.9);

        // 4. 位置评分（偏向中心）
        int dist = std::abs(p.r - 7) + std::abs(p.c - 7);
        score += (30 - dist);

        if (score > bestScore) {
            bestScore = score;
            bestMove = p;
        }
    }

    action.pos = bestMove;
    return action;
}
