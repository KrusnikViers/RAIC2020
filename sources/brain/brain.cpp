#include "brain/brain.h"

#include <assert.h>

namespace {

int dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

}  // namespace

Action Brain::update(const PlayerView& view, DebugInterface* debug) {
  view_ = &view;
  maybeInit();
  updateStored();

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != id_) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        if (builders_count_ <= ranged_count_ + 5 &&
            builders_count_ <= melee_count_ + 5) {
          result.entityActions[entity.id] = EntityAction(
              nullptr,
              std::make_shared<BuildAction>(
                  BUILDER_UNIT,
                  Vec2Int(entity.position.x + 5, entity.position.y + 4)),
              nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
        }
        break;
      }

      case MELEE_BASE: {
        if (melee_count_ <= ranged_count_ + 3) {
          result.entityActions[entity.id] = EntityAction(
              nullptr,
              std::make_shared<BuildAction>(
                  MELEE_UNIT,
                  Vec2Int(entity.position.x + 5, entity.position.y + 4)),
              nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
        }
        break;
      }

      case RANGED_BASE: {
        result.entityActions[entity.id] = EntityAction(
            nullptr,
            std::make_shared<BuildAction>(
                RANGED_UNIT,
                Vec2Int(entity.position.x + 5, entity.position.y + 4)),
            nullptr, nullptr);
        break;
      }

      case BUILDER_UNIT: {
        const Player* me = nullptr;
        for (const auto& player : view.players) {
          if (player.id == id_) me = &player;
        }
        if (builder_id_ == entity.id) {
          const Entity* unbuilt_house = nullptr;
          for (const auto& house : view.entities) {
            if (house.entityType == HOUSE && *house.playerId == id_ &&
                house.health < 50 &&
                (!unbuilt_house ||
                 dist(entity.position, house.position) <=
                     dist(entity.position, unbuilt_house->position))) {
              unbuilt_house = &house;
            }
          }

          if (unbuilt_house) {
            result.entityActions[entity.id] =
                EntityAction(std::make_shared<MoveAction>(
                                 unbuilt_house->position, true, true),
                             nullptr, nullptr,
                             std::make_shared<RepairAction>(unbuilt_house->id));
            break;
          } else if (me && me->resource >= 100) {
            const auto new_house = getNearestPlacing(entity.position);
            if (new_house.x != -1) {
              result.entityActions[entity.id] = EntityAction(
                  std::make_shared<MoveAction>(new_house, true, true),
                  std::make_shared<BuildAction>(HOUSE, new_house), nullptr,
                  nullptr);
              break;
            }
          }
        }

        if (assigned_.count(entity.id)) break;
        const auto* resource = getNearest(entity.position, Resource, true);
        if (resource) {
          result.entityActions[entity.id] = EntityAction(
              std::make_shared<MoveAction>(resource->position, true, true),
              nullptr,
              std::make_shared<AttackAction>(
                  std::make_shared<int>(resource->id), nullptr),
              nullptr);
          if (assigned_.count(entity.id)) targeted_.erase(assigned_[entity.id]);
          assigned_[entity.id]    = resource->id;
          targeted_[resource->id] = entity.id;
        }
        break;
      }

      case RANGED_UNIT:
      case MELEE_UNIT: {
        const auto* nearest_enemy =
            getNearest(entity.position, EnemyUnit, false);
        if (nearest_enemy) {
          result.entityActions[entity.id] = EntityAction(
              std::make_shared<MoveAction>(nearest_enemy->position, true, true),
              nullptr,
              std::make_shared<AttackAction>(
                  std::make_shared<int>(nearest_enemy->id), nullptr),
              nullptr);
          if (assigned_.count(entity.id)) targeted_.erase(assigned_[entity.id]);
          assigned_[entity.id]         = nearest_enemy->id;
          targeted_[nearest_enemy->id] = entity.id;
        }
        break;
      }
    }
  }

  return result;
}

const Entity* Brain::getNearest(Vec2Int pos, Brain::InternalType type,
                                bool unassigned) {
  bool found         = false;
  const Entity* best = nullptr;
  int best_distance  = 0;
  for (const auto& entity : view_->entities) {
    if (type == Resource && entity.entityType != RESOURCE) continue;
    if (type == EnemyUnit && entity.entityType == RESOURCE) continue;
    if (type == EnemyUnit && *entity.playerId == id_) continue;
    if (unassigned && targeted_[entity.id]) continue;

    const int current_distance = dist(entity.position, pos);
    if (!found || current_distance < best_distance) {
      best_distance = current_distance;
      best          = &entity;
      found         = true;
    }
  }
  return best;
}

Vec2Int Brain::getNearestPlacing(Vec2Int position) {
  Vec2Int best_result(-1, -1);
  bool found = false;
  for (size_t x = 2; x < map_.size() - 5; ++x)
    for (size_t y = 2; y < map_.size() - 5; ++y) {
      if (found && dist(Vec2Int(static_cast<int>(x), static_cast<int>(y)),
                        position) >= dist(best_result, position))
        continue;
      bool good = true;
      for (int x_o = -2; x_o <= 5; ++x_o)
        for (int y_o = -2; y_o <= 5; ++y_o) {
          const Entity* cell = map_[x + x_o][y + y_o];
          if (cell == nullptr) continue;
          if (cell->entityType == RESOURCE || cell->entityType == HOUSE ||
              cell->entityType == BUILDER_BASE ||
              cell->entityType == MELEE_BASE ||
              cell->entityType == RANGED_BASE) {
            good = false;
            break;
          }
        }
      if (good) {
        best_result = Vec2Int(static_cast<int>(x), static_cast<int>(y));
        found       = true;
      }
    }
  return best_result;
}

void Brain::updateStored() {
  std::unordered_set<int> entities_alive;
  for (auto& row : map_) {
    for (auto& cell : row) cell = nullptr;
  }

  ranged_count_ = melee_count_ = builders_count_ = 0;
  int builder_met                                = -1;

  for (const auto& entity : view_->entities) {
    entities_alive.insert(entity.id);
    map_[entity.position.x][entity.position.y] = &entity;
    if (entity.entityType == HOUSE) {
      for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
          map_[entity.position.x + i][entity.position.y + j] = &entity;
    }
    if (entity.entityType == BUILDER_BASE || entity.entityType == MELEE_BASE ||
        entity.entityType == RANGED_BASE) {
      for (size_t i = 0; i < 5; ++i)
        for (size_t j = 0; j < 5; ++j)
          map_[entity.position.x + i][entity.position.y + j] = &entity;
    }

    if (entity.playerId && *entity.playerId == id_) {
      if (entity.entityType == BUILDER_UNIT) {
        ++builders_count_;
        builder_met = entity.id;
      } else if (entity.entityType == RANGED_UNIT)
        ++ranged_count_;
      else if (entity.entityType == MELEE_UNIT)
        ++melee_count_;
    }
  }

  if (!entities_alive.count(builder_id_)) builder_id_ = builder_met;

  for (auto it = assigned_.begin(); it != assigned_.end();) {
    auto current = it;
    it           = ++it;
    if (!entities_alive.count(current->first) ||
        !entities_alive.count(current->second)) {
      targeted_.erase(current->second);
      assigned_.erase(current);
    }
  }
}

void Brain::maybeInit() {
  if (id_ != -1) return;
  id_ = view_->myId;
  map_.resize(view_->mapSize);
  for (auto& row : map_) row.resize(view_->mapSize, nullptr);
}

void Brain::debug(const PlayerView& playerView,
                  DebugInterface& debugInterface) {}