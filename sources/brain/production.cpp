#include "brain/production.h"

#include <iostream>

namespace {

int resource_dispatched = 0;

Vec2Int spawnableCell(const Entity* entity) {
  int score      = -1;
  Vec2Int result = entity->position;
  for (const auto& point : frameCells(entity, false)) {
    if (isFree(point.x, point.y) && !cell(point).position_taken) {
      int new_score = point.x + point.y;
      if (new_score > score) {
        score  = new_score;
        result = point;
      }
    }
  }
  cell(result).position_taken = true;
  return result;
}

int unitCost(EntityType type) {
  return props()[type].initialCost +
         (props()[type].canMove ? (int)state().my(type).size() : 0);
}

bool canMakeUnit(EntityType type) {
  return state().resource - resource_dispatched >= unitCost(type) &&
         state().supply_used < state().supply_now;
}

bool needDrone() {
  if (!canMakeUnit(DRONE)) return false;

  static const int drones_limit = 55;
  if (state().my(DRONE).size() >= drones_limit) return false;

  // Too few resources left to make more drones.
  if (state().supply_now > 80 &&
      state().visible_resource <
          state().my(DRONE).size() * 4 * props()[RESOURCE].maxHealth) {
    return false;
  }

  // If we have no barracks yet, build only drones.
  if (!state().has(BARRACKS)) return true;

  // From 20-40 to 60 drones should be build on par with ranged.
  if (state().my(DRONE).size() <= state().my(RANGED).size() + 30) return true;

  // If no conditions met, we're done.
  return false;
}  // namespace

bool needRanged() {
  if (!canMakeUnit(RANGED)) return false;

  // Drones already have priority, so not limit for that one.
  return true;
}

bool needSupply() {
  if (state().resource - resource_dispatched < props()[SUPPLY].initialCost)
    return false;

  static const int supply_limit = lround(map().size * 1.5);
  if (state().supply_now + state().supply_building >= supply_limit)
    return false;

  // Do not place any homes until barracks are set.
  if (state().supply_now >= 40 && !state().has(BARRACKS)) return false;

  // Better keep saving for rush when current one is built.
  if (state().resource < state().supply_building * 20) return false;

  if (state().supply_now * 0.8 < state().supply_used) return true;

  return false;
}

}  // namespace

void ProductionPlanner::update() {
  // Make an assumption that workers are busy digging, to plan slightly ahead.
  resource_dispatched = -(int)state().my(DRONE).size();

  // We need a base all right. Builders will try to do that ASAP, so just
  // return.
  if (!state().has(BASE)) return;

  if (!state().has(BARRACKS) && state().supply_now >= 20) {
    // If we are here and do not have resources -- keep saving!
    state().production_queue.insert(BARRACKS);
    resource_dispatched += unitCost(BARRACKS);
  }

  if (needDrone()) {
    resource_dispatched += unitCost(DRONE);
    state().production_queue.insert(DRONE);
  }

  if (needRanged()) {
    resource_dispatched += unitCost(RANGED);
    state().production_queue.insert(RANGED);
  }

  if (needSupply()) {
    resource_dispatched += unitCost(SUPPLY);
    state().production_queue.insert(SUPPLY);
  }
}

EntityAction ProductionPlanner::command(const Entity* entity) {
  switch (entity->entityType) {
    case BASE:
      if (state().production_queue.count(DRONE)) {
        return EntityAction(
            nullptr,
            std::make_shared<BuildAction>(DRONE, spawnableCell(entity)),
            nullptr, nullptr);
      }
      return kNoAction;

    case BARRACKS:
      if (state().production_queue.count(RANGED)) {
        return EntityAction(
            nullptr,
            std::make_shared<BuildAction>(RANGED, spawnableCell(entity)),
            nullptr, nullptr);
      }
      return kNoAction;

    default:
      return kNoAction;
  }
}
