#pragma once
#include "enemy/ghost.h"

// Red: chases Pac-Man directly
class Blinky : public Ghost {
public:
    Blinky() : Ghost("Blinky", {112.f, 92.f}) {}
protected:
    bool startsOutside() const override { return true; }
    sf::Vector2f chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const override;
    sf::Vector2f scatterTarget() const override { return {25.f, -3.f}; }   // top right
};