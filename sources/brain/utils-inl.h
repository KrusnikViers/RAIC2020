#pragma once

#include "model/Model.hpp"

inline int dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

inline int remoteness(const Vec2Int& p) { return p.x * p.x + p.y * p.y; }
