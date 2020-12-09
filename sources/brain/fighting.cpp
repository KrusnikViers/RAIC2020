#include "brain/fighting.h"

#include "brain/utils-inl.h"

namespace {

bool isOut(const State& state, int x, int y) {
  return x < 0 || y < 0 || x >= state.map.size() || y >= state.map.size();
}

bool isFree(const State& state, int x, int y, bool or_unit = true) {
  if (isOut(state, x, y)) return false;
  const Entity* entity = state.map[x][y];
  return !entity || (or_unit && state.props.at(entity->entityType).canMove);
}

const int kProtectedAreaFromDrone     = 10;
const int kProtectedAreaFromBuildings = 10;

}  // namespace

void FightingPlanner::update(const PlayerView& view, State& state) {
  state_ = &state;

  // Regenerate per-tick stuff
  if (map_.empty()) {
    map_.resize(state.map.size());
    for (auto& row : map_) row.resize(state.map.size());
  }
  for (auto& row : map_) {
    for (auto& cell : row) {
      cell.threat     = Neutral;
      cell.guard_post = false;
    }
  }
  fillHeatMap();

  attackers_.clear();

  for (const auto* enemy : state.enemies) {
    if (map_[enemy->position.x][enemy->position.y].threat == Protected)
      attackers_[enemy->id] = enemy;
  }
  std::unordered_set<int> awaken;
  for (const auto* unit : state.battle_units) {
    if (guard_awaken_.count(unit->id) || !attackers_.empty() ||
        state_->supply_now > 60)
      awaken.insert(unit->id);
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

Vec2Int FightingPlanner::whereToSpawn(const Entity* building) {
  const int size = state_->props.at(building->entityType).size;
  Vec2Int result = building->position;
  bool found     = false;
  result.x += 5;
  result.y += 4;

  for (int i = -1; i < size + 1; ++i) {
    for (int j = -1; j < size + 1; ++j) {
      if (i != -1 && i != size && j != -1 && j != size) continue;
      if ((i == -1 && j == -1) || (i == -1 && j == size) ||
          (i == size && j == -1) || (i == size && j == size))
        continue;

      if (isFree(*state_, building->position.x + i, building->position.y + j,
                 false)) {
        result = Vec2Int(building->position.x + i, building->position.y + j);
        found  = true;
        break;
      }
    }
    if (found) break;
  }

  return result;
}

const Entity* FightingPlanner::getNearestEnemy(const Entity* unit, bool guard) {
  const Entity* result = nullptr;
  int best_score       = 0;
  bool found           = false;
  for (const auto* enemy : state_->enemies) {
    int score = std::lround(r_dist(unit->position, enemy->position));
    if (attackers_.count(enemy->id)) score -= (int)state_->map.size() / 4;
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

  for (int i = 0; i < map_.size(); ++i) {
    if (i % 4 < 2) continue;
    for (int j = 0; j < map_.size(); ++j) {
      if (j % 4 < 2) continue;
      if (state_->map[i][j] && state_->map[i][j]->id != unit->id) continue;
      if (map_[i][j].guard_post) continue;
      if (map_[i][j].threat == Neutral) continue;

      bool is_good = true;
      for (int io = -1; io <= 1; ++io) {
        for (int jo = -1; jo <= 1; ++jo) {
          if (!isFree(*state_, i + io, j + jo)) {
            is_good = false;
            break;
          }
        }
      }
      if (!is_good) continue;

      int distance =
          std::max(result.x, result.y) + dist(Vec2Int(i, j), unit->position);
      if (!found || distance < best_result) {
        best_result = distance;
        result      = Vec2Int(i, j);
        found       = true;
      }
    }
  }

  map_[result.x][result.y].guard_post = true;
  return result;
}

void FightingPlanner::fillHeatMap() {
  for (const auto* entity : state_->buildings) {
    const int size = state_->props.at(entity->entityType).size;
    Vec2Int smallest(
        std::max(0, entity->position.x - kProtectedAreaFromBuildings),
        std::max(0, entity->position.y - kProtectedAreaFromBuildings));
    Vec2Int largest(
        std::min((int)map_.size(),
                 entity->position.x + size + kProtectedAreaFromBuildings + 1),
        std::min((int)map_.size(),
                 entity->position.y + size + kProtectedAreaFromBuildings + 1));
    for (int i = smallest.x; i < largest.x; ++i) {
      for (int j = smallest.y; j < largest.y; ++j) {
        map_[i][j].threat = Protected;
      }
    }
  }

  for (const auto& drone : state_->drones) {
    Vec2Int largest(
        std::min((int)map_.size(), drone->position.x + kProtectedAreaFromDrone),
        std::min((int)map_.size(),
                 drone->position.y + kProtectedAreaFromDrone));
    for (int i = 0; i < largest.x; ++i) {
      for (int j = 0; j < largest.y; ++j) {
        map_[i][j].threat = Protected;
      }
    }
  }
}
