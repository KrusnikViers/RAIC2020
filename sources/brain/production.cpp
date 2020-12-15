#include "brain/production.h"

namespace {

int drones_limit = -1;
int supply_limit = -1;

Vec2Int spawnableCell(const Entity* entity) {
  for (const auto& point : frameCells(entity, false)) {
    if (isFree(point.x, point.y) && state().cell(point).purpose == NoPurpose)
      return point;
  }
  return entity->position;
}

}  // namespace

void ProductionPlanner::update() {
  if (drones_limit == -1) {
    drones_limit = lround(state().map_size * 0.75);
    supply_limit = lround(state().map_size * 1.5);
  }

  if (!state().has(BARRACKS) && state().supply_now >= 20) {
    state().barracks_required = true;
  }

  const int planned_supply = state().supply_now + state().supply_building;
  if (planned_supply < supply_limit) {
    if (!state().barracks_required || state().resource >= 600) {
      if (state().resource >= 50 * state().supply_building &&
          state().supply_used >= 0.8 * state().supply_now) {
        state().supply_required = true;
      }
    }
  }
}

EntityAction ProductionPlanner::command(const Entity* entity) {
  switch (entity->entityType) {
    case BASE:
      if (state().my(DRONE).size() >= drones_limit) return kNoAction;
      if (state().supply_now <= 20 ||
          state().my(DRONE).size() + 40 < state().my(RANGED).size()) {
        return EntityAction(
            nullptr,
            std::make_shared<BuildAction>(DRONE, spawnableCell(entity)),
            nullptr, nullptr);
      }
      return kNoAction;

    case BARRACKS:
      return EntityAction(
          nullptr, std::make_shared<BuildAction>(RANGED, spawnableCell(entity)),
          nullptr, nullptr);

    default:
      return kNoAction;
  }
}
