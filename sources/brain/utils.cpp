#include "brain/utils.h"

#include "brain/state.h"

bool isOut(int x, int y) {
  static const int map_size = state().map_size;
  return x < 0 || y < 0 || x >= map_size || y >= map_size;
}

bool isFree(int x, int y, IsFreeAllowance allowance) {
  if (isOut(x, y)) return false;

  const Entity* entity = state().cell(x, y).entity;
  if (!entity) return true;

  if (!state().mine(entity)) return false;
  if (allowance == AllowDrone && entity->entityType == DRONE) return true;
  if (allowance == AllowUnit && state().props.at(entity->entityType).canMove)
    return true;

  return false;
}

std::vector<Vec2Int> frameCells(const Entity* entity, bool with_corners) {
  return frameCells(entity->position.x, entity->position.y,
                    state().props.at(entity->entityType).size, with_corners);
}

std::vector<Vec2Int> frameCells(int x, int y, int size, bool with_corners) {
  static const std::vector<Vec2Int> offsets = {
      {1, 0}, {0, 1}, {-1, 0}, {0, -1}};

  std::vector<Vec2Int> result;
  result.reserve((size + (with_corners ? 1 : 0)) * 4);

  Vec2Int point(x - 1, y - 1);
  for (const auto& offset : offsets) {
    for (int step = 0; step < size + 1; ++step) {
      point.x += offset.x;
      point.y += offset.y;
      if (!with_corners && (step == size)) continue;
      result.push_back(point);
    }
  }
  return result;
}

std::vector<Vec2Int> nearestCells(int x, int y, int radius) {
  std::vector<Vec2Int> result;
  for (int i = x - radius; i <= x + radius; ++i) {
    for (int j = y - radius; j <= y + radius; ++j) {
      if (m_dist(x, y, i, j) <= radius && !isOut(i, j))
        result.emplace_back(i, j);
    }
  }
  return result;
}

std::vector<Vec2Int> nearestCells(Vec2Int pos, int radius) {
  return nearestCells(pos.x, pos.y, radius);
}

std::shared_ptr<MoveAction> actionMove(Vec2Int position, bool find_nearest) {
  return std::make_shared<MoveAction>(position, find_nearest, true);
}

std::shared_ptr<AttackAction> actionAttack(int target) {
  return std::make_shared<AttackAction>(std::make_shared<int>(target), nullptr);
}
