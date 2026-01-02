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
    ctx.history.clear();
    
    // 黑棋从天元 (7, 7) 开始
    Pos center = {7, 7};
    board.set(center, Side::Black);
    Action firstAction = Action{ActionType::Place, center, std::chrono::milliseconds(0)};
    ctx.lastAction = firstAction;
    ctx.history.push_back({Side::Black, firstAction});

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
                reason = "规则: 白方第一手必须下在自己的半场（行 >= 7）。";
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
        // 如果处于等待禁手申诉阶段，而白棋落子（Place），则视为放弃申诉。
        if (ctx.phase == Phase::PendingClaim) {
            ctx.phase = Phase::Normal;
            ctx.pendingForbidden = false;
        }

        board.set(action.pos.value(), side);
        ctx.lastAction = action;
        ctx.turnIndex++;
        ctx.toMove = (side == Side::Black) ? Side::White : Side::Black;
        
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

    // 4 个方向
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
            outcome.reason = isOverline ? "白方长连 (胜)" : "白方五连";
            return outcome;
        }
    } else {
        if (isFive) {
            // 五连优先：即使是禁手，只要成五就算赢。
            outcome.status = GameStatus::Win;
            outcome.winner = Side::Black;
            outcome.reason = "黑方五连";
            return outcome;
        }
        
        // 禁手检查逻辑
        std::string forbiddenReason;
        if (isForbidden(board, p, forbiddenReason)) {
            // 等待申诉的状态。
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

// 禁手
bool GomokuRuleSet::isForbidden(const Board& board, Pos p, std::string& reason) const {
    // 1. 长连
    if (checkOverline(board, p)) {
        reason = "长连 (6+)";
        return true;
    }
    
    // 2. 三三
    if (checkThreeThree(board, p)) {
        reason = "三三禁手";
        return true;
    }
    
    // 3. 四四
    if (checkFourFour(board, p)) {
        reason = "四四禁手";
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
    // 向后看 4 格
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

// 检查在 p 点落子是否在方向 (dr, dc) 上形成活三
// 活三必须能够形成直四。
// 模式 (1=己方, 0=空):
// 01110 (标准)
// 010110 (跳三)
// 011010 (跳三)
bool GomokuRuleSet::isOpenThree(const Board& board, Pos p, int dr, int dc) const {
    // 中心在模式向量的索引 4 处。
    std::vector<int> line = getLinePattern(board, p, dr, dc);
    
    //寻找以 4 为中心（刚刚下了 '1' 的地方）的特定模式
    // 索引 4 处的棋子是 '1'
    
    // 活三的模式（必须两端都不受阻）
    // "01110" -> 索引 3,4,5 是 1。2 和 6 是 0
    // "011010" -> 索引 3,4,6 是 1。5 是 0。2 和 7 是 0
    // "010110" -> 索引 2,4,5 是 1。3 是 0。1 和 6 是 0
    
    // 注意：模式向量大小为 9。索引 0..8。中心是 4。
    
    // 1. 连三: .XXX.
    // 检查是否有以 4 为中心的 111。
    // 可能是 3,4,5 处的 111 -> 检查 2=0, 6=0
    // 可能是 4,5,6 处的 111 -> 检查 3=0, 7=0
    // p 永远在 4
    
    // 情况 A: 111 (p 在中间或两端)
    // 如果 p 在中间: 1(p)1 -> 3=1, 4=1, 5=1. 端点 2=0, 6=0
    if (line[3]==1 && line[4]==1 && line[5]==1) {
        if (line[2]==0 && line[6]==0) return true;
    }
    // 如果 p 在左边: (p)11 -> 4=1, 5=1, 6=1. 端点 3=0, 7=0
    if (line[4]==1 && line[5]==1 && line[6]==1) {
        if (line[3]==0 && line[7]==0) return true;
    }
    // 如果 p 在右边: 11(p) -> 2=1, 3=1, 4=1. 端点 1=0, 5=0
    if (line[2]==1 && line[3]==1 && line[4]==1) {
        if (line[1]==0 && line[5]==0) return true;
    }
    
    // 情况 B: 跳三 1011 或 1101
    // 1(p)01 -> 4=1, 5=0, 6=1, 7=1. 端点 3=0, 8=0.
    if (line[4]==1 && line[5]==0 && line[6]==1 && line[7]==1) {
        if (line[3]==0 && line[8]==0) return true;
    }
    // 10(p)1 -> 3=1, 4=0 (不可能, p 是 1), ...
    // p 是 1.
    // 1011 其中 p 是单独的 1: (p)011 -> 4=1, 5=0, 6=1, 7=1. (同上)
    // 1011 其中 p 在 11 中: 10(p)1 -> 2=1, 3=0, 4=1, 5=1. 端点 1=0, 6=0
    if (line[2]==1 && line[3]==0 && line[4]==1 && line[5]==1) {
        if (line[1]==0 && line[6]==0) return true;
    }
    // 1011 其中 p 是末端: 101(p) -> 1=1, 2=0, 3=1, 4=1. 端点 0=0, 5=0
    if (line[1]==1 && line[2]==0 && line[3]==1 && line[4]==1) {
        if (line[0]==0 && line[5]==0) return true;
    }
    
    // 1101 的对称情况
    // 110(p) -> 2=1, 3=1, 4=0 (不可能)
    // (p)101 -> 4=1, 5=1, 6=0, 7=1. 端点 3=0, 8=0
    if (line[4]==1 && line[5]==1 && line[6]==0 && line[7]==1) {
        if (line[3]==0 && line[8]==0) return true;
    }
    // 1(p)01 -> 3=1, 4=1, 5=0, 6=1. 端点 2=0, 7=0
    if (line[3]==1 && line[4]==1 && line[5]==0 && line[6]==1) {
        if (line[2]==0 && line[7]==0) return true;
    }
    
    return false;
}

// 检查在 p 点落子是否形成四
// 必须不是长连 (6+)
// 必须能够变成 5
bool GomokuRuleSet::isFour(const Board& board, Pos p, int dr, int dc) const {
    std::vector<int> line = getLinePattern(board, p, dr, dc);
    
    int countFours = 0;
    
    for (int start = 0; start <= 4; ++start) {
        int end = start + 4;
        if (end > 8) continue;
        
        // 检查这个 5 格窗口
        int ones = 0;
        int zeros = 0;
        int zeroIdx = -1;
        
        for (int k = start; k <= end; ++k) {
            if (line[k] == 1) ones++;
            else if (line[k] == 0) {
                zeros++;
                zeroIdx = k;
            }
        }
        
        if (ones == 4 && zeros == 1) {
            return true;
        }
    }
    
    return false;
}
