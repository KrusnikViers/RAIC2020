#include "brain/fighting.h"

#include "brain/utils.h"

namespace {

const int kProtectedAreaFromDrone     = 10;
const int kProtectedAreaFromBuildings = 15;

}  // namespace

void FightingPlanner::update() {
  // Regenerate per-tick stuff
  fillHeatMap();

  attackers_.clear();
  for (const auto* enemy : state().enemies) {
    if (state().map[enemy->position.x][enemy->position.y].protection_class ==
        State::Protected)
      attackers_[enemy->id] = enemy;
  }
  std::unordered_set<int> awaken;
  for (const auto* unit : state().battle_units) {
    if (guard_awaken_.count(unit->id) || !attackers_.empty() ||
        state().resources.size() < state().initial_resource * 0.66) {
      awaken.insert(unit->id);
    }
  }
  guard_awaken_ = awaken;
}

EntityAction FightingPlanner::command(const Entity* entity) {
  if (guard_awaken_.count(entity->id)) {
    const Entity* enemy = getNearestEnemy(entity, false);
    if (!enemy) {
      return EntityAction(nullptr, nullptr,
                          std::make_shared<AttackAction>(
                              nullptr, std::make_shared<AutoAttack>(
                                           50, std::vector<EntityType>())),
                          nullptr);
    }
    targeted_[enemy->id] = enemy;
    return EntityAction(
        std::make_shared<MoveAction>(enemy->position, true, true), nullptr,
        std::make_shared<AttackAction>(
            std::make_shared<int>(enemy->id),
            std::make_shared<AutoAttack>(10, std::vector<EntityType>())),
        nullptr);
  } else {
    Vec2Int new_pos = getBestGuardPosition(entity);
    if (new_pos == entity->position)
      return EntityAction(nullptr, nullptr, nullptr, nullptr);
    return EntityAction(
        std::make_shared<MoveAction>(new_pos, true, true), nullptr,
        std::make_shared<AttackAction>(
            nullptr,
            std::make_shared<AutoAttack>(10, std::vector<EntityType>())),
        nullptr);
  }
}

const Entity* FightingPlanner::getNearestEnemy(const Entity* unit, bool guard) {
  const Entity* result = nullptr;
  int best_score       = 0;
  bool found           = false;
  for (const auto* enemy : state().enemies) {
    int score = std::lround(r_dist(unit->position, enemy->position)) +
                (enemy->entityType == BUILDER_UNIT ? 10 : 0);
    if (attackers_.count(enemy->id)) {
      score -= (state().map_size - lr_dist(enemy->position, Vec2Int(0, 0))) *
               (state().map_size - lr_dist(enemy->position, Vec2Int(0, 0)));
    }
    if (!found || score < best_score) {
      found      = true;
      best_score = score;
      result     = enemy;
    }
  }
  return result;
}

Vec2Int FightingPlanner::getBestGuardPosition(const Entity* unit) {
  Vec2Int result  = unit->position;
  bool found      = false;
  int best_result = std::max(result.x, result.y);

  for (int i = 0; i < state().map.size(); ++i) {
    if (i % 4 < 2) continue;
    for (int j = 0; j < state().map.size(); ++j) {
      if (j % 4 < 2) continue;
      if (state().map[i][j].entity && state().map[i][j].entity->id != unit->id)
        continue;
      if (state().map[i][j].guard_planned_position) continue;
      if (state().map[i][j].drone_planned_position) continue;
      if (state().map[i][j].protection_class == State::Neutral) continue;

      bool is_good = true;
      for (int io = -1; io <= 1; ++io) {
        for (int jo = -1; jo <= 1; ++jo) {
          if (!isFree(i + io, j + jo)) {
            is_good = false;
            break;
          }
        }
      }
      if (!is_good) continue;

      int distance =
          std::max(result.x, result.y) + m_dist(Vec2Int(i, j), unit->position);
      if (!found || distance < best_result) {
        best_result = distance;
        result      = Vec2Int(i, j);
        found       = true;
      }
    }
  }

  state().map[result.x][result.y].guard_planned_position = true;
  return result;
}

void FightingPlanner::fillHeatMap() {
  for (const auto* entity : state().buildings) {
    const int size = state().props.at(entity->entityType).size;
    Vec2Int smallest(
        std::max(0, entity->position.x - kProtectedAreaFromBuildings),
        std::max(0, entity->position.y - kProtectedAreaFromBuildings));
    Vec2Int largest(
        std::min((int)state().map.size(),
                 entity->position.x + size + kProtectedAreaFromBuildings + 1),
        std::min((int)state().map.size(),
                 entity->position.y + size + kProtectedAreaFromBuildings + 1));
    for (int i = smallest.x; i < largest.x; ++i) {
      for (int j = smallest.y; j < largest.y; ++j) {
        state().map[i][j].protection_class = State::Protected;
      }
    }
  }

  for (const auto& drone : state().drones) {
    Vec2Int largest(std::min((int)state().map.size(),
                             drone->position.x + kProtectedAreaFromDrone),
                    std::min((int)state().map.size(),
                             drone->position.y + kProtectedAreaFromDrone));
    for (int i = 0; i < largest.x; ++i) {
      for (int j = 0; j < largest.y; ++j) {
        state().map[i][j].protection_class = State::Protected;
      }
    }
  }
}
