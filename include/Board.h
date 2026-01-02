#pragma once
#include "Common.h"
#include <vector>
#include <array>

class Board {
public:
    static const int SIZE = 15;

    Board();
    
    void reset();
    bool isValid(Pos p) const;
    bool isEmpty(Pos p) const;
    bool isFull() const;
    
    Side get(Pos p) const;
    void set(Pos p, Side s);
    void clear(Pos p); // For undo or testing

    // Helper for checking lines
    // Returns count of consecutive stones of 'side' starting from p in direction (dr, dc)
    // Does NOT include p itself if includeSelf is false.
    int countConsecutive(Pos p, int dr, int dc, Side side) const;

private:
    std::array<std::array<Side, SIZE>, SIZE> grid;
    int stoneCount;
};
