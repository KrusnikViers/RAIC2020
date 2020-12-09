#include "brain/brain.h"

Action Brain::update(const PlayerView& view, DebugInterface* debug) {
  state_.update(view);
  fighting_.update(view, state_);
  building_.update(view, state_);

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != state_.id) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        if ((state_.supply_now < 20 && state_.drones.size() < 8) ||
            (state_.drones.size() < state_.supply_now * 0.7 &&
             state_.resource < 150)) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               BUILDER_UNIT, fighting_.whereToSpawn(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
        }
        break;
      }

      case MELEE_BASE: {
        if (state_.melees.size() <= state_.ranged.size() + 3) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               MELEE_UNIT, fighting_.whereToSpawn(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
        }
        break;
      }

      case RANGED_BASE: {
        result.entityActions[entity.id] =
            EntityAction(nullptr,
                         std::make_shared<BuildAction>(
                             RANGED_UNIT, fighting_.whereToSpawn(&entity)),
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
