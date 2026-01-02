#include "../include/GameEngine.h"
#include "../include/GomokuRuleSet.h"
#include "../include/HumanPlayer.h"
#include "../include/AIPlayer.h"
#include <iostream>
#include <future>
#include <thread>
#include <conio.h> // For _kbhit, _getch

GameEngine::GameEngine() {
    rules = std::make_unique<GomokuRuleSet>();
}

void GameEngine::setup() {
    // flush terminal
    std::cout << "\033[2J\033[H";
    
    std::cout << "Gomoku terminal demo\n";
    std::cout << "Select Mode:\n";
    std::cout << "1. Human vs Human\n";
    std::cout << "2. Human vs AI (Human is Black)\n";
    std::cout << "3. AI vs Human (Human is White)\n";
    std::cout << "Choice: ";
    
    int choice;
    if (!(std::cin >> choice)) choice = 1;
    std::cin.ignore();

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
    
    auto gameStart = std::chrono::steady_clock::now();
    ctx.totalGameDurationSeconds = 1800; // 30 分钟

    while (running) {
        // 全局计时
        auto nowGlobal = std::chrono::steady_clock::now();
        ctx.elapsedGameSeconds = std::chrono::duration_cast<std::chrono::seconds>(nowGlobal - gameStart).count();
        
        // 超时检测
        if (ctx.elapsedGameSeconds >= ctx.totalGameDurationSeconds) {
            renderer.render(ctx, board, "Time limit reached (30 min). Taking a 10 minute break...", "");
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate break start
            
            // 加时逻辑
            std::cout << "\n\n[System] 10 Minute Rest Period Complete.\n";
            std::cout << "Select Overtime:\n1. 5 Minutes\n2. 10 Minutes\nChoice: ";
            
            int otChoice;
            // Clear input buffer
            while (_kbhit()) _getch();
            
            std::cin >> otChoice;
            std::cin.ignore();
            
            if (otChoice == 1) ctx.totalGameDurationSeconds += 300;
            else ctx.totalGameDurationSeconds += 600;
            
            message = "Overtime Started!";
            // Reset loop to re-check time and render
            continue;
        }

        Player* currentPlayer = (ctx.toMove == Side::Black) ? blackPlayer.get() : whitePlayer.get();
        bool isHuman = (currentPlayer->name() == "Human");
        
        Action action;
        bool actionReceived = false;
        
        if (isHuman) {
            // Polling 等待玩家输入
            std::string currentInput = "";
            auto periodStart = std::chrono::steady_clock::now();
            const int timeLimitSeconds = 15;
            int lastRemaining = -1;
            
            while (!actionReceived) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - periodStart).count();
                int remaining = timeLimitSeconds - elapsed;
                if (remaining < 0) remaining = 0;
                
                // 更新时间
                ctx.elapsedGameSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - gameStart).count();

                // Redraw if time changed or input changed (we redraw on input below)
                if (remaining != lastRemaining || (ctx.elapsedGameSeconds % 60 == 0)) { // Update at least every second
                    std::string timeMsg = message + " [Move Time: " + std::to_string(remaining) + "s]";
                    renderer.render(ctx, board, timeMsg, currentInput);
                    lastRemaining = remaining;
                }

                // Check input
                if (_kbhit()) {
                    int ch = _getch();
                    if (ch == '\r' || ch == '\n') {
                        action = HumanPlayer::parseCommand(currentInput);
                        actionReceived = true;
                        std::cout << "\n";
                    } else if (ch == '\b' || ch == 127) {
                        if (!currentInput.empty()) {
                            currentInput.pop_back();
                            // redraw
                            lastRemaining = -1; 
                        }
                    } else if (ch >= 32 && ch <= 126) { // Printable
                        currentInput += (char)ch;
                        //redraw
                        lastRemaining = -1;
                    }
                }
                
                if (elapsed >= timeLimitSeconds && !actionReceived) {
                     // Timeout
                    Outcome outcome = rules->onTimeout(ctx, ctx.toMove);
                    message = outcome.reason;
                    if (outcome.status == GameStatus::TimeoutLose) {
                        renderer.render(ctx, board, "TIMEOUT LOSE! " + message, currentInput);
                        running = false;
                        std::cout << "\nGame Over (Timeout). Press Enter to exit.\n";
                        return; 
                    } else {
                        // Warning
                        periodStart = std::chrono::steady_clock::now();
                        lastRemaining = -1; 
                        message = "TIMEOUT WARNING! " + outcome.reason;
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        } else {
            // AI Logic (Async)
            renderer.render(ctx, board, message + " (AI Thinking...)", "");
            
            auto future = std::async(std::launch::async, [&]() {
                return currentPlayer->getAction(ctx, board, *rules);
            });
            
            // We can just wait for AI, assuming AI is fast enough or doesn't need strict timeout enforcement like Human
            // Or we can enforce timeout for AI too if needed.
            // For now, just wait.
            action = future.get();
            actionReceived = true;
        }

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
            message = "WARNING: Black played a forbidden move (" + outcome.reason + "). White can 'claim' to win! Type 'claim' to put up your hands.";
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
