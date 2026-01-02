#include "../include/GameEngine.h"
#include "../include/GomokuRuleSet.h"
#include "../include/HumanPlayer.h"
#include "../include/AIPlayer.h"
#include <iostream>
#include <future>
#include <thread>
#include <conio.h> // For _kbhit, _getch
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

GameEngine::GameEngine() {
    rules = std::make_unique<GomokuRuleSet>();
}

void GameEngine::setup() {
    while (true) {
        // 清空终端
        std::cout << "\033[2J\033[H";
        
        std::cout << "Gomoku terminal demo\n";
        std::cout << "Select Mode:\n";
        std::cout << "1. Human vs Human\n";
        std::cout << "2. Human vs AI (Human is Black)\n";
        std::cout << "3. AI vs Human (Human is White)\n";
        std::cout << "4. Load Replay\n";
        std::cout << "Choice: ";
        
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            choice = 1;
        }
        std::cin.ignore();

        if (choice == 4) {
            loadAndReplay();
            continue; // 再次显示菜单
        }

        int aiLevel = 2; // Default Medium
        if (choice == 2 || choice == 3) {
            std::cout << "Select AI Difficulty:\n";
            std::cout << "1. Easy (Fast)\n";
            std::cout << "2. Medium (Balanced)\n";
            std::cout << "3. Hard (Slow)\n";
            std::cout << "Choice: ";
            if (!(std::cin >> aiLevel)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                aiLevel = 2;
            }
            std::cin.ignore();
        }

        if (choice == 1) {
            blackPlayer = std::make_unique<HumanPlayer>();
            whitePlayer = std::make_unique<HumanPlayer>();
        } else if (choice == 2) {
            blackPlayer = std::make_unique<HumanPlayer>();
            whitePlayer = std::make_unique<AIPlayer>(aiLevel);
        } else {
            blackPlayer = std::make_unique<AIPlayer>(aiLevel);
            whitePlayer = std::make_unique<HumanPlayer>();
        }

        board.reset();
        rules->initGame(ctx, board);
        break;
    }
}

void GameEngine::run() {
    bool appRunning = true;
    bool needSetup = true;

    while (appRunning) {
        if (needSetup) {
            setup();
        } else {
            board.reset();
            rules->initGame(ctx, board);
        }

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
            renderer.render(ctx, board, "时间限制已到 (30 分钟). 休息 10 分钟...", "");
            std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟休息开始
            
            // 加时逻辑
            std::cout << "\n\n[系统] 10 分钟休息结束。\n";
            std::cout << "选择加时:\n1. 5 分钟\n2. 10 分钟\n选择: ";
            
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
            // 轮询等待玩家输入
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

                // 如果时间改变或输入改变则重绘（我们在下面输入时重绘）
                if (remaining != lastRemaining || (ctx.elapsedGameSeconds % 60 == 0)) { // 至少每秒更新一次
                    std::string timeMsg = message + " [step time left: " + std::to_string(remaining) + "s]";
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
                     // 超时
                    Outcome outcome = rules->onTimeout(ctx, ctx.toMove);
                    message = outcome.reason;
                    if (outcome.status == GameStatus::TimeoutLose) {
                        renderer.render(ctx, board, "超时判负! " + message, currentInput);
                        running = false;
                        std::cout << "\n游戏结束 (超时). 按回车键退出。\n";
                        break; 
                    } else {
                        // 警告
                        periodStart = std::chrono::steady_clock::now();
                        lastRemaining = -1; 
                        message = "超时警告! " + outcome.reason;
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

        if (action.type == ActionType::Undo) {
            // Undo Logic
            int undoSteps = 0;
            bool isPvE = (blackPlayer->name() == "AI" || whitePlayer->name() == "AI");
            
            if (isPvE) {
                // In PvE, undo 2 steps to get back to human turn
                // Unless only 1 move has been made (e.g. Black Human played, then undo immediately before White AI moves? 
                // No, AI moves immediately. So usually we are at turnIndex N.
                // If it's Human's turn, it means AI just moved? No.
                // Wait. If it is Human's turn, the LAST move was AI.
                // So if Human says "undo", they want to undo AI's move AND their own previous move.
                undoSteps = 2;
            } else {
                // PvP: Undo 1 step
                undoSteps = 1;
            }

            if (ctx.history.size() < undoSteps) {
                message = "Cannot undo: Not enough history.";
                continue;
            }

            for (int i = 0; i < undoSteps; ++i) {
                auto last = ctx.history.back();
                ctx.history.pop_back();
                if (last.second.type == ActionType::Place && last.second.pos.has_value()) {
                    board.clear(last.second.pos.value());
                }
                // Revert context
                ctx.turnIndex--;
                ctx.toMove = (ctx.toMove == Side::Black) ? Side::White : Side::Black;
                // Reset warnings/phase if needed? 
                // Simplifying: Just keep warnings. Phase might need revert if we crossed opening.
                if (ctx.turnIndex <= 2) ctx.phase = Phase::Opening;
                else ctx.phase = Phase::Normal;
                ctx.pendingForbidden = false; // Clear pending claim
            }
            
            message = "Undo successful.";
            continue;
        }

        // 落子后的判定逻辑
        std::string reason;
        if (!rules->validateAction(ctx, board, ctx.toMove, action, reason)) {
            message = "Invalid Action: " + reason;
            continue;
        }

        Side justMoved = ctx.toMove;

        rules->applyAction(ctx, board, justMoved, action);
        ctx.history.push_back(std::make_pair(justMoved, action));

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
    
        // Post-game menu
        bool inMenu = true;
        while (inMenu) {
            std::cout << "\n[Game Over Menu]\n";
            std::cout << "1. Play Again (Same Settings)\n";
            std::cout << "2. Switch Mode (New Settings)\n";
            std::cout << "3. Save Game Record\n";
            std::cout << "4. Exit\n";
            std::cout << "Choice: ";
            
            int choice;
            while (_kbhit()) _getch(); // Clear buffer
            
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                choice = 0;
            }
            std::cin.ignore(); 

            if (choice == 1) {
                needSetup = false;
                inMenu = false;
            } else if (choice == 2) {
                needSetup = true;
                inMenu = false;
            } else if (choice == 3) {
                saveGameRecord();
                std::cout << "Press Enter to continue...";
                std::cin.get();
            } else if (choice == 4) {
                appRunning = false;
                inMenu = false;
            } else {
                std::cout << "Invalid choice.\n";
            }
        }
    }
}

void GameEngine::saveGameRecord() {
    // Save to ../match so it is a sibling of build directory
    std::string dir = "../match";
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
    }

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << dir << "/game_record_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".txt";
    std::string filename = oss.str();

    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "Gomoku Game Record\n";
        outfile << "Date: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
        outfile << "Black: " << (blackPlayer ? blackPlayer->name() : "Unknown") << "\n";
        outfile << "White: " << (whitePlayer ? whitePlayer->name() : "Unknown") << "\n";
        
        outfile << "\n--- Move History ---\n";
        int moveNum = 1;
        for (const auto& move : ctx.history) {
            outfile << moveNum++ << ". " << (move.first == Side::Black ? "Black" : "White");
            if (move.second.type == ActionType::Place && move.second.pos.has_value()) {
                outfile << " (" << move.second.pos->r << "," << move.second.pos->c << ")";
            } else if (move.second.type == ActionType::Resign) {
                outfile << " Resigns";
            } else if (move.second.type == ActionType::ClaimForbidden) {
                outfile << " Claims Forbidden";
            }
            outfile << " [" << move.second.spent.count() << "ms]\n";
        }

        outfile << "\n--- Final Board ---\n";
        outfile << "   ";
        for (int c = 0; c < Board::SIZE; ++c) outfile << (char)('A' + c) << " ";
        outfile << "\n";
        
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
            outfile << (r + 1 < 10 ? " " : "") << (r + 1) << " ";
            for (int c = 0; c < Board::SIZE; ++c) {
                Side s = board.get({r, c});
                std::string symbol;
                if (s == Side::Black) symbol = "○";
                else if (s == Side::White) symbol = "●";
                else symbol = getGridChar(r, c);
                
                outfile << symbol;
                if (c < Board::SIZE - 1) outfile << "─";
            }
            outfile << "\n";
        }
        outfile.close();
        std::cout << "Game saved to " << filename << "\n";
    } else {
        std::cout << "Failed to save game.\n";
    }
}

void GameEngine::loadAndReplay() {
    std::string dir = "../match";
    if (!fs::exists(dir)) {
        std::cout << "No match records found.\n";
        std::cout << "Press Enter to return.\n";
        std::cin.get();
        return;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".txt") {
            files.push_back(entry.path().string());
        }
    }

    if (files.empty()) {
        std::cout << "No match records found.\n";
        std::cout << "Press Enter to return.\n";
        std::cin.get();
        return;
    }

    std::cout << "Select Record:\n";
    for (size_t i = 0; i < files.size(); ++i) {
        std::cout << (i + 1) << ". " << fs::path(files[i]).filename().string() << "\n";
    }
    std::cout << "Choice: ";
    int choice;
    if (!(std::cin >> choice) || choice < 1 || choice > (int)files.size()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return;
    }
    std::cin.ignore();

    std::string filepath = files[choice - 1];
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cout << "Failed to open file.\n";
        return;
    }

    // Parse moves
    std::vector<std::pair<Side, Pos>> moves;
    std::string line;
    bool inHistory = false;
    while (std::getline(infile, line)) {
        if (line.find("--- Move History ---") != std::string::npos) {
            inHistory = true;
            continue;
        }
        if (line.find("--- Final Board ---") != std::string::npos) {
            break;
        }
        if (inHistory && !line.empty()) {
            // Format: "1. Black (7,7) [0ms]"
            // Simple parsing
            size_t openParen = line.find('(');
            size_t comma = line.find(',');
            size_t closeParen = line.find(')');
            
            if (openParen != std::string::npos && comma != std::string::npos && closeParen != std::string::npos) {
                std::string rStr = line.substr(openParen + 1, comma - openParen - 1);
                std::string cStr = line.substr(comma + 1, closeParen - comma - 1);
                
                try {
                    int r = std::stoi(rStr);
                    int c = std::stoi(cStr);
                    Side s = (line.find("Black") != std::string::npos) ? Side::Black : Side::White;
                    moves.push_back({s, {r, c}});
                } catch (...) {}
            }
        }
    }
    infile.close();

    // Replay Loop
    Board replayBoard;
    int currentStep = 0;
    bool replaying = true;
    
    while (replaying) {
        // Reconstruct board up to currentStep
        replayBoard.reset();
        for (int i = 0; i < currentStep; ++i) {
            if (i < moves.size()) {
                replayBoard.set(moves[i].second, moves[i].first);
            }
        }

        // Render
        GameContext dummyCtx; // Just for rendering
        dummyCtx.toMove = (currentStep < moves.size()) ? moves[currentStep].first : Side::None;

        // manually render or reuse renderer with a custom message.
        std::string msg = "Replay Mode: Step " + std::to_string(currentStep) + "/" + std::to_string(moves.size());
        msg += " | [<-] Prev  [->] Next  [Q] Quit";
        renderer.render(dummyCtx, replayBoard, msg, "");

        // Input
        int ch = _getch();
        if (ch == 0 || ch == 224) { // Arrow keys
            ch = _getch();
            if (ch == 75) { // Left
                if (currentStep > 0) currentStep--;
            } else if (ch == 77) { // Right
                if (currentStep < moves.size()) currentStep++;
            }
        } else if (ch == 'q' || ch == 'Q') {
            replaying = false;
        }
    }
}
