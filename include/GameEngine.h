#pragma once
#include "Common.h"
#include "Board.h"
#include "RuleSet.h"
#include "Player.h"
#include "Renderer.h"
#include <memory>

class GameEngine {
public:
    GameEngine();
    void run();

private:
    Board board;
    GameContext ctx;
    std::unique_ptr<RuleSet> rules;
    std::unique_ptr<Player> blackPlayer;
    std::unique_ptr<Player> whitePlayer;
    Renderer renderer;

    void setup();
    void loop();
    Action getPlayerActionWithTimeout(Player* player, int timeoutSeconds);
};
