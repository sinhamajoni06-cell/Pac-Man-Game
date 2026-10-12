#include "enemy/blinky.h"

sf::Vector2f Blinky::chaseTarget(const GhostContext& ctx, sf::Vector2f) const {
    return tileOf(ctx.player.pos);                  // straight at Pac-Man
}