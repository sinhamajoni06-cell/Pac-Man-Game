#pragma once
#include "enemy/ghost.h"

// Orange: chases when far away, runs to his corner when close
class Clyde : public Ghost {
public:
    Clyde() : Ghost("Clyde", {128.f, 116.f}) {}
protected:
    sf::Vector2f chaseTarget(const GhostContext& ctx, sf::Vector2f myTile) const override;
    sf::Vector2f scatterTarget() const override { return {0.f, 31.f}; }    // bottom left
};