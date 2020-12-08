#include "brain/brain.h"

namespace {

int dist(const Vec2Int& p1, const Vec2Int& p2) {
  return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

}  // namespace

Action Brain::update(const PlayerView& view, DebugInterface* debug) {
  view_ = &view;
  id_   = view_->myId;
  state_.update(*view_);
  building_queue_.update(view, state_);

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != id_) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        if (state_.drones.size() <= state_.ranged.size() + 5 &&
            state_.drones.size() <= state_.melees.size() + 5) {
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
        if (state_.melees.size() <= state_.ranged.size() + 3) {
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

      case BUILDER_UNIT:
        result.entityActions[entity.id] =
            building_queue_.commandsForBuilder(&entity);
        break;

      case RANGED_UNIT:
      case MELEE_UNIT: {
        const auto* nearest_enemy = getNearestEnemy(entity.position);
        if (nearest_enemy) {
          result.entityActions[entity.id] = EntityAction(
              std::make_shared<MoveAction>(nearest_enemy->position, true, true),
              nullptr,
              std::make_shared<AttackAction>(
                  std::make_shared<int>(nearest_enemy->id), nullptr),
              nullptr);
        }
        break;
      }
    }
  }

  return result;
}

const Entity* Brain::getNearestEnemy(Vec2Int pos) {
  bool found         = false;
  const Entity* best = nullptr;
  int best_distance  = 0;
  for (const auto& entity : view_->entities) {
    if (entity.entityType == RESOURCE) continue;
    if (*entity.playerId == id_) continue;

    const int current_distance = dist(entity.position, pos);
    if (!found || current_distance < best_distance) {
      best_distance = current_distance;
      best          = &entity;
      found         = true;
    }
  }
  return best;
}
