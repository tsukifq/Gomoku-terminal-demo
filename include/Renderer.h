#pragma once
#include "Common.h"
#include "Board.h"
#include <string>

class Renderer {
public:
    void render(const GameContext& ctx, const Board& board, const std::string& message = "");
    void clearScreen();
};
