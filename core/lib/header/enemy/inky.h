#pragma once
#include "enemy/ghost.h"

// Cyan: uses Blinky's position to flank Pac-Man
class Inky : public Ghost {
public:
    Inky() : Ghost("Inky", {96.f, 116.f}) {}
protected:
    sf::Vector2f chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const override;
    sf::Vector2f scatterTarget() const override { return {27.f, 31.f}; }   // bottom right
};