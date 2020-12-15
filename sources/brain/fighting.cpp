#include "brain/fighting.h"

namespace {

const int kProtectedAreaFromDrone     = 10;
const int kProtectedAreaFromBuildings = 15;

}  // namespace

void FightingPlanner::update() {}

EntityAction FightingPlanner::command(const Entity* entity) {
  const Entity* enemy = getNearestEnemy(entity);
  if (enemy) {
    return EntityAction(actionMove(enemy->position, true), nullptr,
                        actionAttack(enemy->id), nullptr);
  }
  return EntityAction(actionMove(getLeastKnownPosition(entity), true), nullptr,
                      nullptr, nullptr);
}

const Entity* FightingPlanner::getNearestEnemy(const Entity* unit) {
  const Entity* result = nullptr;
  int best_score       = -1;
  for (const auto* enemy : state().enemies) {
    int score = m_dist(unit->position, enemy->position) +
                (enemy->entityType == DRONE ? 15 : 0);
    if (best_score == -1 || score < best_score) {
      best_score = score;
      result     = enemy;
    }
  }
  return result;
}

Vec2Int FightingPlanner::getLeastKnownPosition(const Entity* unit) {
  int score = -1;
  Vec2Int best_result(state().map_size / 2, state().map_size / 2);
  for (int i = 0; i < state().map_size; ++i) {
    for (int j = 0; j < state().map_size; ++j) {
      const int last_visible = state().cell(i, j).last_visible;
      if (last_visible && (score == -1 || last_visible >= score)) {
        score       = last_visible;
        best_result = Vec2Int(i, j);
      }
    }
  }
  return best_result;
}
