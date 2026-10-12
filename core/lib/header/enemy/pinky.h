#pragma once
#include "enemy/ghost.h"

// Pink: ambushes, aims 4 tiles ahead of Pac-Man
class Pinky : public Ghost {
public:
    Pinky() : Ghost("Pinky", {112.f, 116.f}) {}
protected:
    sf::Vector2f chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const override;
    sf::Vector2f scatterTarget() const override { return {2.f, -3.f}; }    // top left
};