#include "enemy/pinky.h"

sf::Vector2f Pinky::chaseTarget(const GhostContext& ctx, sf::Vector2f) const {
    return ahead(ctx.player, 4);                    // 4 tiles in front of Pac-Man
}