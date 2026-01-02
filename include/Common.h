#pragma once
#include <string>
#include <optional>
#include <chrono>
#include <vector>

// Basic types
enum class Side { Black, White, None };

struct Pos {
    int r, c;
    bool operator==(const Pos& other) const { return r == other.r && c == other.c; }
};

enum class ActionType { Place, Resign, ClaimForbidden, OfferDraw, AcceptDraw, RejectDraw };

struct Action {
    ActionType type;
    std::optional<Pos> pos; // Only for Place
    std::chrono::milliseconds spent;
};

enum class GameStatus { Ongoing, Win, Draw, Forbidden, TimeoutLose, PendingClaim };

struct Outcome {
    GameStatus status;
    std::optional<Side> winner;
    std::string reason; // "five", "double-three", "timeout", etc.
};

enum class Phase { Opening, Normal, PendingClaim, Finished };

struct GameContext {
    Side toMove = Side::Black;
    int turnIndex = 0;
    std::optional<Action> lastAction;
    int blackTimeoutWarnings = 0;
    int whiteTimeoutWarnings = 0;
    Phase phase = Phase::Opening;
    
    // Time control
    long long totalGameDurationSeconds = 1800; // 30 minutes default
    long long elapsedGameSeconds = 0;

    // For pending forbidden claim
    // If Black plays a forbidden move, we store it here.
    // If White claims, White wins. If White plays elsewhere, this is cleared.
    bool pendingForbidden = false; 
};
