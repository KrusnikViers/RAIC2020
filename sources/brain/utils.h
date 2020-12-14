#pragma once

#include <cmath>

#include "model/Model.hpp"

inline int m_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

inline int m_dist(int x1, int y1, int x2, int y2) {
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

inline double r_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) +
                   (p1.y - p2.y) * (p1.y - p2.y));
}

inline int lr_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::lround(r_dist(p1, p2));
}

inline int remoteness(const Vec2Int& p) { return p.x * p.x + p.y * p.y; }

enum IsFreeAllowance { None, Drone, Unit };
bool isFree(int x, int y, IsFreeAllowance allowance = None);
bool isOut(int x, int y);

std::vector<Vec2Int> frameCells(int x, int y, int size,
                                bool with_corners = true);
std::vector<Vec2Int> frameCells(const Entity* entity, bool with_corners = true);

std::vector<Vec2Int> nearestCells(int x, int y, int radius);
std::vector<Vec2Int> nearestCells(Vec2Int pos, int radius);

inline Vec2Int spawnableCell(const Entity* entity) {
  for (const auto& point : frameCells(entity, false)) {
    if (isFree(point.x, point.y)) return point;
  }
  return entity->position;
}

const EntityAction kNoAction = EntityAction(nullptr, nullptr, nullptr, nullptr);
std::shared_ptr<MoveAction> actionMove(Vec2Int position, bool find_nearest);
std::shared_ptr<AttackAction> actionAttack(int target);