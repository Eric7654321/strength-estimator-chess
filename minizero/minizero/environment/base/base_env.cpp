#include "base_env.h"

namespace minizero::env {

char playerToChar(Player p)
{
#if CHESS
    switch (p) {
        case Player::kPlayerNone: return 'N';
        case Player::kPlayer1: return 'W';
        case Player::kPlayer2: return 'B';
        default: return '?';
    }
#else
    switch (p) {
        case Player::kPlayerNone: return 'N';
        case Player::kPlayer1: return 'B';
        case Player::kPlayer2: return 'W';
        default: return '?';
    }
#endif
}

Player charToPlayer(char c)
{
#if CHESS
    switch (c) {
        case 'B':
        case 'b': return Player::kPlayer2;
        case 'W':
        case 'w': return Player::kPlayer1;
        default: return Player::kPlayerSize;
    }
#else
    switch (c) {
        case 'N': return Player::kPlayerNone;
        case 'B':
        case 'b': return Player::kPlayer1;
        case 'W':
        case 'w': return Player::kPlayer2;
        default: return Player::kPlayerSize;
    }
#endif
}

Player getNextPlayer(Player player, int num_player)
{
    if (num_player == 1) {
        return player;
    } else if (num_player == 2) {
        return (player == Player::kPlayer1 ? Player::kPlayer2 : Player::kPlayer1);
    }

    return Player::kPlayerNone;
}

Player getPreviousPlayer(Player player, int num_player)
{
    if (num_player <= 2) { return getNextPlayer(player, num_player); }
    return Player::kPlayerNone;
}

} // namespace minizero::env
