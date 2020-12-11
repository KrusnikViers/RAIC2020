#include "brain/brain.h"

namespace {

Vec2Int spawnableCell(const Entity* entity) {
  for (const auto& point : frameCells(entity, false)) {
    if (isFree(point.x, point.y)) return point;
  }
  return entity->position;
}

}  // namespace

Action Brain::update(const PlayerView& view) {
  state().update(view);
  building_.update();
  fighting_.update();

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != state().id) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        if ((state().supply_now < 20 && state().drones.size() < 8) ||
            (state().drones.size() < state().supply_now * 0.75 &&
             state().resource < 200)) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               BUILDER_UNIT, spawnableCell(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
        }
        break;
      }

      case MELEE_BASE: {
        if (state().melees.size() <= state().ranged.size() + 5) {
          result.entityActions[entity.id] = EntityAction(
              nullptr,
              std::make_shared<BuildAction>(MELEE_UNIT, spawnableCell(&entity)),
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
            std::make_shared<BuildAction>(RANGED_UNIT, spawnableCell(&entity)),
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
