#include "../include/HumanPlayer.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

Action HumanPlayer::getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) {
    Action action;
    action.spent = std::chrono::milliseconds(0); // Engine measures time usually, but we can set 0

    std::string input;
    // std::cout << "Enter move (e.g. H8), 'claim', or 'q': "; 
    // Output is handled by Renderer usually, but prompt is needed.
    // We'll assume Renderer printed the prompt.
    
    if (!std::getline(std::cin, input)) {
        action.type = ActionType::Resign; // EOF
        return action;
    }

    // Trim
    input.erase(0, input.find_first_not_of(" \t\n\r"));
    input.erase(input.find_last_not_of(" \t\n\r") + 1);
    
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "q" || lower == "quit") {
        action.type = ActionType::Resign;
        return action;
    }
    if (lower == "claim") {
        action.type = ActionType::ClaimForbidden;
        return action;
    }
    if (lower == "draw") {
        action.type = ActionType::OfferDraw;
        return action;
    }

    // Parse coordinate: Letter + Number (e.g., H8 or 8H)
    // Standard: Column Letter, Row Number.
    // A=0, B=1, ... O=14.
    // 1=0, ... 15=14.
    
    int r = -1, c = -1;
    
    // Find letter and number parts
    std::string letters, digits;
    for (char ch : input) {
        if (std::isalpha(ch)) letters += std::toupper(ch);
        else if (std::isdigit(ch)) digits += ch;
    }
    
    if (letters.empty() || digits.empty()) {
        // Invalid
        // We can loop here or return invalid action and let Engine ask again?
        // Better to loop here for UX, but Engine might handle "Invalid action".
        // Let's return a dummy Place action that is invalid.
        action.type = ActionType::Place;
        action.pos = {-1, -1}; 
        return action;
    }
    
    c = letters[0] - 'A';
    try {
        r = std::stoi(digits) - 1; // 1-based to 0-based
    } catch (...) {
        r = -1;
    }
    
    // In Gomoku, usually rows are numbered from bottom (1) to top (15) or top to bottom?
    // Standard Renju: 15 at top, 1 at bottom? Or matrix style?
    // Matrix style (1 at top) is easier for CLI.
    // Let's assume 1 is top row (index 0).
    
    action.type = ActionType::Place;
    action.pos = {r, c};
    
    return action;
}
