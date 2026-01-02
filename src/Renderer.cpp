#include "../include/Renderer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void Renderer::clearScreen() {
    // Do nothing here, handled in render with ANSI codes
}

void Renderer::render(const GameContext& ctx, const Board& board, const std::string& message, const std::string& currentInput) {
    std::stringstream ss;
    
    // Move cursor to home (0,0)
    ss << "\033[H";

    ss << "   ";
    for (int c = 0; c < Board::SIZE; ++c) {
        ss << (char)('A' + c) << " ";
    }
    ss << "\033[K\n"; // Clear rest of line

    auto getGridChar = [](int r, int c) -> std::string {
        if (r == 0) {
            if (c == 0) return "┌";
            if (c == Board::SIZE - 1) return "┐";
            return "┬";
        }
        if (r == Board::SIZE - 1) {
            if (c == 0) return "└";
            if (c == Board::SIZE - 1) return "┘";
            return "┴";
        }
        if (c == 0) return "├";
        if (c == Board::SIZE - 1) return "┤";
        if (r == 7 && c == 7) return "╋";
        return "┼";
    };

    for (int r = 0; r < Board::SIZE; ++r) {
        ss << std::setw(2) << (r + 1) << " "; // 1-based index
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            Side s = board.get(p);
            
            bool isLast = false;
            if (ctx.lastAction.has_value() && ctx.lastAction->type == ActionType::Place && ctx.lastAction->pos.has_value()) {
                if (ctx.lastAction->pos.value() == p) isLast = true;
            }

            std::string symbol;
            if (s == Side::Black) {
                symbol = "○";
            } else if (s == Side::White) {
                symbol = "●";
            } else {
                symbol = getGridChar(r, c);
            }

            if (isLast) {
                // 最后一步特殊显示：使用亮洋红色 (Bold Magenta)
                ss << "\033[1;35m" << symbol << "\033[0m"; 
            } else {
                ss << symbol;
            }

            if (c < Board::SIZE - 1) {
                ss << "─";
            }
        }
        ss << "\033[K\n"; // Clear rest of line
    }

    ss << "\033[K\nTurn: " << ctx.turnIndex << " | To Move: " << (ctx.toMove == Side::Black ? "Black (○)" : "White (●)") << "\033[K\n";
    
    // Format Global Time
    long long elapsed = ctx.elapsedGameSeconds;
    long long total = ctx.totalGameDurationSeconds;
    ss << "Global Time: " << (elapsed / 60) << ":" << std::setw(2) << std::setfill('0') << (elapsed % 60) 
       << " / " << (total / 60) << ":" << std::setw(2) << std::setfill('0') << (total % 60) << "\033[K\n";

    ss << "Warnings - Black: " << ctx.blackTimeoutWarnings << "/3 | White: " << ctx.whiteTimeoutWarnings << "/3\033[K\n";
    if (ctx.phase == Phase::PendingClaim) {
        ss << "STATUS: PENDING CLAIM! White can type 'claim' to win.\033[K\n";
    }
    
    if (!message.empty()) {
        ss << "Message: " << message << "\033[K\n";
    }
    
    ss << "Command: " << currentInput << "\033[K";
    
    // Clear from cursor to end of screen (in case output shrank)
    ss << "\033[J";

    std::cout << ss.str() << std::flush;
}
