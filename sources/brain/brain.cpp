#include "brain/brain.h"

const double kRangedBuildPriority = 1.1;

Action Brain::update(const PlayerView& view) {
  state().update(view);
  building_.update();
  fighting_.update();

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!entity.playerId || *entity.playerId != state().id) continue;

    switch (entity.entityType) {
      case BUILDER_BASE: {
        const bool total_upper_limit =
            state().drones.size() >= state().map_size * 0.66 ||
            state().drones.size() >= state().resources.size() / 3;
        const bool start_rush =
            state().supply_now == 15 && state().drones.size() < 9;

        if (!total_upper_limit && (!fighting_.recovery || start_rush)) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               BUILDER_UNIT, spawnableCell(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] = kNoAction;
        }
        break;
      }

      case MELEE_BASE: {
        double efficacy = (double)(state().props[RANGED_UNIT].initialCost +
                                   state().ranged.size()) /
                          (double)(state().props[MELEE_UNIT].initialCost +
                                   state().melees.size());
        const bool cheap_enough =
            state().battle_units.size()<fighting_.needed_army &&
            efficacy >
            kRangedBuildPriority;

        if (fighting_.critical_fight || cheap_enough) {
          result.entityActions[entity.id] = EntityAction(
              nullptr,
              std::make_shared<BuildAction>(MELEE_UNIT, spawnableCell(&entity)),
              nullptr, nullptr);
        } else {
          result.entityActions[entity.id] = kNoAction;
        }
        break;
      }

      case RANGED_BASE: {
        if (state().battle_units.size() < fighting_.needed_army ||
            state().resource > 250) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               RANGED_UNIT, spawnableCell(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] = kNoAction;
        }
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
