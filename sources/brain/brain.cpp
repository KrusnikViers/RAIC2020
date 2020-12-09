#include "brain/brain.h"

Action Brain::update(const PlayerView& view, DebugInterface* debug) {
  state_.update(view);
  building_.update(view, state_);
  fighting_.update(view, state_);

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != state_.id) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        if ((state_.supply_now < 20 && state_.drones.size() < 8) ||
            (state_.drones.size() < state_.supply_now / 2 &&
             state_.resource < 100)) {
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
        result.entityActions[entity.id] = building_.command(&entity);
        break;

      case RANGED_UNIT:
      case MELEE_UNIT:
        result.entityActions[entity.id] = fighting_.command(&entity);
        break;
    }
  }

  return result;
}
