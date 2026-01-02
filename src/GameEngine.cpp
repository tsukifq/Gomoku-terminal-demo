#include "../include/GameEngine.h"
#include "../include/GomokuRuleSet.h"
#include "../include/HumanPlayer.h"
#include "../include/AIPlayer.h"
#include <iostream>
#include <future>
#include <thread>

GameEngine::GameEngine() {
    rules = std::make_unique<GomokuRuleSet>();
}

void GameEngine::setup() {
    std::cout << "Gomoku - C++ Final Assignment\n";
    std::cout << "Select Mode:\n";
    std::cout << "1. Human vs Human\n";
    std::cout << "2. Human vs AI (Human is Black)\n";
    std::cout << "3. AI vs Human (Human is White)\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(); // Consume newline

    if (choice == 1) {
        blackPlayer = std::make_unique<HumanPlayer>();
        whitePlayer = std::make_unique<HumanPlayer>();
    } else if (choice == 2) {
        blackPlayer = std::make_unique<HumanPlayer>();
        whitePlayer = std::make_unique<AIPlayer>();
    } else {
        blackPlayer = std::make_unique<AIPlayer>();
        whitePlayer = std::make_unique<HumanPlayer>();
    }

    board.reset();
    rules->initGame(ctx, board);
}

void GameEngine::run() {
    setup();

    bool running = true;
    std::string message = "Game Start!";

    while (running) {
        renderer.render(ctx, board, message);

        Player* currentPlayer = (ctx.toMove == Side::Black) ? blackPlayer.get() : whitePlayer.get();
        
        Action action;
        bool actionReceived = false;

        // 用一个异步的thread来获取玩家输入
        auto future = std::async(std::launch::async, [&]() {
            return currentPlayer->getAction(ctx, board, *rules);
        });

        auto periodStart = std::chrono::steady_clock::now();
        const int timeLimitSeconds = 15;
        int lastRemaining = -1;

        // 计时逻辑
        while (!actionReceived) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - periodStart).count();
            int remaining = timeLimitSeconds - elapsed;
            if (remaining < 0) remaining = 0;

            if (remaining != lastRemaining) {
                std::string timeMsg = message + " [Time: " + std::to_string(remaining) + "s]";
                renderer.render(ctx, board, timeMsg);
                lastRemaining = remaining;
            }

            std::future_status status = future.wait_for(std::chrono::milliseconds(100));

            if (status == std::future_status::ready) {
                action = future.get();
                actionReceived = true;
            } else if (elapsed >= timeLimitSeconds) {
                // Timeout occurred
                Outcome outcome = rules->onTimeout(ctx, ctx.toMove);
                message = outcome.reason;
                
                if (outcome.status == GameStatus::TimeoutLose) {
                    renderer.render(ctx, board, "TIMEOUT LOSE! " + message);
                    running = false;
                    std::cout << "Game Over (Timeout). Press Enter to exit.\n";
                    return; 
                } else {
                    // Just a warning
                    periodStart = std::chrono::steady_clock::now();
                    lastRemaining = -1; 
                }
            }
        }

        // 超时中止
        if (!running) break;

        // 落子后的判定逻辑
        std::string reason;
        if (!rules->validateAction(ctx, board, ctx.toMove, action, reason)) {
            message = "Invalid Action: " + reason;
            continue;
        }

        Side justMoved = ctx.toMove;

        rules->applyAction(ctx, board, justMoved, action);

        Outcome outcome = rules->evaluateAfterAction(ctx, board, justMoved, action);
        
        if (outcome.status == GameStatus::Win) {
            renderer.render(ctx, board, "WINNER: " + (outcome.winner == Side::Black ? std::string("Black") : std::string("White")) + " (" + outcome.reason + ")");
            running = false;
        } else if (outcome.status == GameStatus::Draw) {
            renderer.render(ctx, board, "DRAW: " + outcome.reason);
            running = false;
        } else if (outcome.status == GameStatus::Forbidden) {
             renderer.render(ctx, board, "FORBIDDEN MOVE CLAIMED! White Wins. (" + outcome.reason + ")");
             running = false;
        } else if (outcome.status == GameStatus::PendingClaim) {
            message = "WARNING: Black played a forbidden move (" + outcome.reason + "). White can 'claim' to win!";
            ctx.phase = Phase::PendingClaim;
            ctx.pendingForbidden = true;
        } else {
            message = "";
        }
    }
    
    std::cout << "Game Over. Press Enter to exit.\n";
    // std::cin.ignore();
    std::cin.get();
}
