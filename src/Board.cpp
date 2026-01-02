#include "../include/Board.h"

Board::Board() {
    reset();
}

void Board::reset() {
    for (auto& row : grid) {
        row.fill(Side::None);
    }
    stoneCount = 0;
}

bool Board::isValid(Pos p) const {
    return p.r >= 0 && p.r < SIZE && p.c >= 0 && p.c < SIZE;
}

bool Board::isEmpty(Pos p) const {
    return isValid(p) && grid[p.r][p.c] == Side::None;
}

bool Board::isFull() const {
    return stoneCount >= SIZE * SIZE;
}

Side Board::get(Pos p) const {
    if (!isValid(p)) return Side::None;
    return grid[p.r][p.c];
}

void Board::set(Pos p, Side s) {
    if (isValid(p)) {
        if (grid[p.r][p.c] == Side::None && s != Side::None) stoneCount++;
        else if (grid[p.r][p.c] != Side::None && s == Side::None) stoneCount--;
        grid[p.r][p.c] = s;
    }
}

void Board::clear(Pos p) {
    set(p, Side::None);
}

int Board::countConsecutive(Pos p, int dr, int dc, Side side) const {
    int count = 0;
    int r = p.r + dr;
    int c = p.c + dc;
    while (isValid({r, c}) && get({r, c}) == side) {
        count++;
        r += dr;
        c += dc;
    }
    return count;
}
