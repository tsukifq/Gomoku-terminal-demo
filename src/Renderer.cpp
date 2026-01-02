#include "../include/Renderer.h"
#include <iostream>
#include <iomanip>

void Renderer::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void Renderer::render(const GameContext& ctx, const Board& board, const std::string& message) {
    clearScreen();
    
    std::cout << "   ";
    for (int c = 0; c < Board::SIZE; ++c) {
        std::cout << (char)('A' + c) << " ";
    }
    std::cout << "\n";

    for (int r = 0; r < Board::SIZE; ++r) {
        std::cout << std::setw(2) << (r + 1) << " "; // 1-based index
        for (int c = 0; c < Board::SIZE; ++c) {
            Pos p = {r, c};
            Side s = board.get(p);
            
            bool isLast = false;
            if (ctx.lastAction.has_value() && ctx.lastAction->type == ActionType::Place && ctx.lastAction->pos.has_value()) {
                if (ctx.lastAction->pos.value() == p) isLast = true;
            }

            // 用小写区分最近一次落子
            if (s == Side::Black) {
                std::cout << (isLast ? "x " : "X ");
            } else if (s == Side::White) {
                std::cout << (isLast ? "o " : "O ");
            } else {
                if (p.r == 7 && p.c == 7) std::cout << "+ "; // Center
                else std::cout << ". ";
            }
        }
        std::cout << "\n";
    }

    std::cout << "\nTurn: " << ctx.turnIndex << " | To Move: " << (ctx.toMove == Side::Black ? "Black (X)" : "White (O)") << "\n";
    std::cout << "Warnings - Black: " << ctx.blackTimeoutWarnings << "/3 | White: " << ctx.whiteTimeoutWarnings << "/3\n";
    if (ctx.phase == Phase::PendingClaim) {
        std::cout << "STATUS: PENDING CLAIM! White can type 'claim' to win.\n";
    }
    if (!message.empty()) {
        std::cout << "Message: " << message << "\n";
    }
    std::cout << "Command: ";
}
