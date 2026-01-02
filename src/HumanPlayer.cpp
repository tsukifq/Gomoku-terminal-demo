#include "../include/HumanPlayer.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

Action HumanPlayer::getAction(const GameContext& ctx, const Board& board, const RuleSet& rules) {
    Action action;
    action.spent = std::chrono::milliseconds(0); 

    std::string input;
    if (!std::getline(std::cin, input)) {
        action.type = ActionType::Resign; // EOF
        return action;
    }
    return parseCommand(input);
}

Action HumanPlayer::parseCommand(const std::string& input) {
    Action action;
    action.spent = std::chrono::milliseconds(0);

    std::string lower = input;
    // Trim
    size_t first = lower.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        // Empty
        action.type = ActionType::Place;
        action.pos = {-1, -1};
        return action;
    }
    size_t last = lower.find_last_not_of(" \t\n\r");
    lower = lower.substr(first, (last - first + 1));
    
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
    int r = -1, c = -1;
    
    std::string letters, digits;
    for (char ch : input) { // Use original input for case-insensitive letter check if needed, but A-O is fine
        if (std::isalpha(ch)) letters += std::toupper(ch);
        else if (std::isdigit(ch)) digits += ch;
    }
    
    if (letters.empty() || digits.empty()) {
        action.type = ActionType::Place;
        action.pos = {-1, -1}; 
        return action;
    }
    
    c = letters[0] - 'A';
    try {
        r = std::stoi(digits) - 1; 
    } catch (...) {
        r = -1;
    }
    
    action.type = ActionType::Place;
    action.pos = {r, c};
    
    return action;
}
