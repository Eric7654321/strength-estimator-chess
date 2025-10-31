#pragma once

#include <string>

namespace minizero::env::chess {

class Square {
public:
    Square() {}
    Square(int square) : rank_(square / 8), file_(square % 8), square_(square) {}
    Square(std::string square) : rank_(square[1] - '1'), file_(square[0] - 'a')
    {
        square_ = rank_ * 8 + file_;
    }

    friend bool operator==(const Square& lhs, const int& rhs)
    {
        return lhs.square_ == rhs;
    }

    int rank_;
    int file_;
    int square_;
};

} // namespace minizero::env::chess
