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
  Vec2Int next_estimated_position;
  const auto estimation = estimate(entity, &next_estimated_position);
  if (isOut(next_estimated_position.x, next_estimated_position.y)) {
    next_estimated_position = entity->position;
  }
  std::shared_ptr<MoveAction> move_action;

  const Entity* enemy = getTargetedEnemy(entity);
  if (enemy) {
    return EntityAction(nullptr, nullptr, actionAttack(enemy->id, true),
                        nullptr);
  }

  enemy = getNearestEnemy(entity);
  if (enemy) {
    return EntityAction(estimation == Advance
                            ? map().moveAction(entity, enemy->position)
                            : map().moveAction(entity, next_estimated_position),
                        nullptr, actionAttack(enemy->id, true), nullptr);
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

    if (best_score == -1 || score < best_score) {
      best_score = score;
      result     = enemy;
    }
  }
  return result;
}

FightingPlanner::TacticsDecision FightingPlanner::estimate(
    const Entity* unit, Vec2Int* next_position) {
  typedef std::pair<int, const Entity*> allyUnit;

  Vec2Float enemy_direction(0.f, 0.f);
  double enemy_proximity = 0.f;
  std::vector<const Entity*> direct_enemies;
  std::priority_queue<allyUnit, std::vector<allyUnit>, std::greater<allyUnit>>
      direct_allies;

  for (const Entity* enemy : state().enemies) {
    if ((enemy->entityType == RANGED || enemy->entityType == MELEE) &&
        dist(enemy->position, unit->position) < 10) {
      direct_enemies.push_back(enemy);
      float proximity = 1.f / dist(enemy->position, unit->position);
      enemy_direction.x +=
          sign(enemy->position.x - unit->position.x) * proximity;
      enemy_direction.y +=
          sign(enemy->position.y - unit->position.y) * proximity;
      enemy_proximity += proximity;
    }
  }
  if (direct_enemies.empty()) return Advance;
  *next_position = posOffset(unit->position, Vec2Int(-sign(enemy_direction.x),
                                                     -sign(enemy_direction.y)));
  enemy_proximity /= direct_enemies.size();

  for (const Entity* ally : state().my(RANGED)) {
    direct_allies.push(
        std::make_pair(dist(ally->position, unit->position), ally));
  }
  if (direct_allies.empty()) {
    return Retreat;
  }

  double ally_proximity = 0.f;
  int ally_number =
      std::min((int)direct_enemies.size() + 1, (int)direct_allies.size());
  int counter = ally_number;
  while (counter) {
    --counter;
    double current_ally_proximity = 0.f;
    for (const Entity* enemy : direct_enemies) {
      current_ally_proximity +=
          1.f / dist(enemy->position, direct_allies.top().second->position);
    }
    current_ally_proximity /= direct_enemies.size();
    ally_proximity += current_ally_proximity;
    direct_allies.pop();
  }
  ally_proximity /= ally_number;

  if (enemy_proximity * 0.75 > ally_proximity) {
    return Retreat;
  } else {
    *next_position = unit->position;
    return Advance;
  }
}
