#include "enemy/clyde.h"

sf::Vector2f Clyde::chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const {
    sf::Vector2f pac = tileOf(ctx.player.pos);
    float dx = pac.x - myTile.x, dy = pac.y - myTile.y;
    if (dx * dx + dy * dy > 64.f) return pac;       // farther than 8 tiles: chase
    return scatterTarget();                         // closer: run to his corner
}