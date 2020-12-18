#include "brain/fighting.h"

#include <map>

namespace {

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
    return EntityAction(nullptr, nullptr, actionAttack(enemy->id, true),
                        nullptr);
  }

  Vec2Int tactical = tacticalPosition(entity);
  if (tactical.x != -1) {
    return EntityAction(map().moveAction(entity, tactical), nullptr, nullptr,
                        nullptr);
  }

  enemy = getNearestEnemy(entity);
  if (enemy) {
    return EntityAction(map().moveAction(entity, enemy->position), nullptr,
                        actionAttack(enemy->id, true), nullptr);
  }

  return EntityAction(map().moveAction(entity, map().leastKnownPosition()),
                      nullptr, nullptr, nullptr);
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
    else if (enemy->entityType == BARRACKS)
      score -= 3;

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
    score -= 2 * (map().size - (enemy->position.x + enemy->position.y) / 2);

    if (enemy->entityType == DRONE)
      score -= 10;
    else if (enemy->entityType == BARRACKS)
      score -= 20;
    if (state().threatening_workers.count(enemy->id))
      score -= 2 * state().threatening_workers[enemy->id];

    if (best_score == -1 || score < best_score) {
      best_score = score;
      result     = enemy;
    }
  }
  return result;
}

Vec2Int FightingPlanner::tacticalPosition(const Entity* unit) {
  std::vector<const Entity*> enemies;
  std::vector<const Entity*> allies;
  Vec2Int result(-1, -1);

  int enemy_health = 0, ally_health = 0;
  int enemy_attack = 0, ally_attack = 0;

  for (const Entity* enemy : state().enemies) {
    if (enemy->entityType == RANGED || enemy->entityType == MELEE ||
        enemy->entityType == TURRET) {
      if (dist(unit->position, enemy->position) <= 7) {
        enemy_health += enemy->health;
        enemy_attack += props()[enemy->entityType].attack->damage;
        enemies.push_back(enemy);
      }
    }
  }
  if (enemies.empty()) return Vec2Int(-1, -1);

  for (const Entity* ally : state().my(RANGED)) {
    for (const auto* enemy : enemies) {
      if (dist(ally->position, enemy->position) <= 7) {
        allies.push_back(ally);
        ally_health += ally->health;
        ally_attack += props()[RANGED].attack->damage;
      }
    }
  }

  bool should_advance = (double)ally_health / (double)enemy_attack >=
                        (double)enemy_health / (double)ally_attack;
  if (std::max(unit->position.x, unit->position.y) < 30) should_advance = true;
  auto distance_score = [&](Vec2Int my_position) {
    double score    = 0;
    int my_distance = -1;
    for (const auto* ally : allies) {
      int closest_distance = -1;
      for (const auto* enemy : enemies) {
        int distance = dist(ally->position, enemy->position);
        if (closest_distance == -1 || distance < closest_distance)
          closest_distance = distance;
      }
      score += closest_distance;
    }

    for (const auto* enemy : enemies) {
      int distance = dist(my_position, enemy->position);
      if (my_distance == -1 || distance < my_distance) my_distance = distance;
    }

    return std::make_pair(score / allies.size(), my_distance);
  };

  if (should_advance) {
    result            = unit->position;
    auto score        = distance_score(unit->position);
    double best_score = std::abs(score.first - 0.6 - score.second);
    for (const auto& pos : frameCells(unit, false)) {
      if (isFree(pos.x, pos.y) && !cell(pos).position_taken) {
        score                = distance_score(pos);
        double current_score = std::abs(score.first - 1 - score.second);
        if (current_score < best_score) {
          result     = pos;
          best_score = current_score;
        }
      }
    }
    cell(result).position_taken = true;
  } else {
    result            = unit->position;
    auto score        = distance_score(unit->position);
    double best_score = std::abs(score.first + 1 - score.second);
    for (const auto& pos : frameCells(unit, false)) {
      if (isFree(pos.x, pos.y) && !cell(pos).position_taken) {
        score                = distance_score(pos);
        double current_score = std::abs(score.first + 1 - score.second);
        if (current_score < best_score) {
          result     = pos;
          best_score = current_score;
        }
      }
    }
    cell(result).position_taken = true;
  }

  return result;
}
