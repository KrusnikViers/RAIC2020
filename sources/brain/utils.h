#pragma once

#include <cmath>

#include "model/Model.hpp"

inline int m_dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

inline int m_dist(int x1, int y1, int x2, int y2) {
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

enum IsFreeAllowance { CompletelyFree, AllowDrone, AllowUnit };
bool isFree(int x, int y, IsFreeAllowance allowance = CompletelyFree);
bool isOut(int x, int y);

std::vector<Vec2Int> frameCells(int x, int y, int size,
                                bool with_corners = true);
std::vector<Vec2Int> frameCells(const Entity* entity, bool with_corners = true);

std::vector<Vec2Int> nearestCells(int x, int y, int radius);
std::vector<Vec2Int> nearestCells(Vec2Int pos, int radius);

const EntityAction kNoAction = EntityAction(nullptr, nullptr, nullptr, nullptr);
std::shared_ptr<MoveAction> actionMove(Vec2Int position, bool find_nearest);
std::shared_ptr<AttackAction> actionAttack(int target);
