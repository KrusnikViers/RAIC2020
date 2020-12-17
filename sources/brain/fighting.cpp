#include "brain/fighting.h"

namespace {

const int kProtectedAreaFromDrone     = 10;
const int kProtectedAreaFromBuildings = 15;

int hpRemains(const Entity* enemy) {
  return enemy->health - (state().planned_damage.count(enemy->id)
                              ? state().planned_damage[enemy->id]
                              : 0);
}

}  // namespace

void FightingPlanner::update() {}

EntityAction FightingPlanner::command(const Entity* entity) {
  const Entity* enemy = getTargetedEnemy(entity);
  if (enemy) {
    return EntityAction(nullptr, nullptr, actionAttack(enemy->id), nullptr);
  }

  enemy = getNearestEnemy(entity);
  if (enemy) {
    return EntityAction(actionMove(enemy->position, true), nullptr,
                        actionAttack(enemy->id), nullptr);
  }

  return EntityAction(actionMove(getLeastKnownPosition(entity), true), nullptr,
                      nullptr, nullptr);
}

const Entity* FightingPlanner::getTargetedEnemy(const Entity* unit) {
  if (!state().targeted.count(unit->id)) return nullptr;

  const int attack_power = state().props[unit->entityType].attack->damage;

  int best_score                = -1;
  const Entity* resulting_enemy = nullptr;
  for (const auto* enemy : state().targeted[unit->id]) {
    int score = hpRemains(enemy);
    if (score < 0) score = 1000;
    if (score < attack_power) score -= 500;
    if (enemy->entityType == RANGED)
      score -= 20;
    else if (enemy->entityType == MELEE)
      score -= 10;
    else if (enemy->entityType == DRONE)
      score -= 5;

    if (!resulting_enemy || (score < best_score)) {
      resulting_enemy = enemy;
      best_score      = score;
    }
  }

  if (!state().planned_damage.count(resulting_enemy->id))
    state().planned_damage[resulting_enemy->id] = 0;
  state().planned_damage[resulting_enemy->id] += attack_power;
  return resulting_enemy;
}

const Entity* FightingPlanner::getNearestEnemy(const Entity* unit) {
  const Entity* result = nullptr;
  int best_score       = -1;
  for (const auto* enemy : state().enemies) {
    int score = dist(unit->position, enemy->position);
    if (enemy->entityType == DRONE) score -= 15;

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
