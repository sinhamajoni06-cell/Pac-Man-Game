#include "enemy/inky.h"

sf::Vector2f Inky::chaseTarget(const GhostContext& ctx, sf::Vector2f) const {
    sf::Vector2f pivot  = ahead(ctx.player, 2);     // 2 tiles in front of Pac-Man
    sf::Vector2f blinky = tileOf(ctx.blinkyPos);
    return pivot + (pivot - blinky);                // Blinky -> pivot, doubled
}