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

  const int attack_power = props()[unit->entityType].attack->damage;

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
  Vec2Int best_result(map().size / 2, map().size / 2);
  for (int i = 0; i < map().size; ++i) {
    for (int j = 0; j < map().size; ++j) {
      const int blind_counter = cell(i, j).blind_counter;
      if (blind_counter && (score == -1 || blind_counter >= score)) {
        score       = blind_counter;
        best_result = Vec2Int(i, j);
      }
    }
  }
  return best_result;
}
