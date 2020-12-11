#include "brain/brain.h"

namespace {

Vec2Int spawnableCell(const Entity* entity) {
  for (const auto& point : frameCells(entity, false)) {
    if (isFree(point.x, point.y)) return point;
  }
  return entity->position;
}

const double kRangedBuildPriority = 1.1;

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
        const bool no_more_drones =
            state().drones.size() >= state().map_size * 0.66 ||
            state().drones.size() >= state().resources.size() / 3;
        const bool setup_on_start =
            state().supply_now == 15 && state().drones.size() < 9;

        if (!no_more_drones && (!fighting_.recovery || setup_on_start)) {
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
        double efficacy = (double)(state().props[RANGED_UNIT].initialCost +
                                   state().ranged.size()) /
                          (double)(state().props[MELEE_UNIT].initialCost +
                                   state().melees.size());
        if (fighting_.critical_fight ||
            (state().battle_units.size() < fighting_.needed_army &&
             efficacy > kRangedBuildPriority)) {
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
        if (state().battle_units.size() < fighting_.needed_army ||
            state().resource > 250) {
          result.entityActions[entity.id] =
              EntityAction(nullptr,
                           std::make_shared<BuildAction>(
                               RANGED_UNIT, spawnableCell(&entity)),
                           nullptr, nullptr);
        } else {
          result.entityActions[entity.id] =
              EntityAction(nullptr, nullptr, nullptr, nullptr);
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
