#pragma once

#include <cmath>

#include "model/Model.hpp"

inline int dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

inline int remoteness(const Vec2Int& p) { return p.x * p.x + p.y * p.y; }

inline double r_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) +
                   (p1.y - p2.y) * (p1.y - p2.y));
}
