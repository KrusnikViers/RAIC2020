#pragma once

#include <cmath>

#include "model/Model.hpp"

inline int m_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

inline double r_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) +
                   (p1.y - p2.y) * (p1.y - p2.y));
}

inline int remoteness(const Vec2Int& p) { return p.x * p.x + p.y * p.y; }

bool isOut(int x, int y);

enum IsFreeAllowance { None, Drone, Unit };
bool isFree(int x, int y, IsFreeAllowance allowance = None);

std::vector<Vec2Int> frameCells(int x, int y, int size,
                                bool with_corners = true);
std::vector<Vec2Int> frameCells(const Entity* entity, bool with_corners = true);
