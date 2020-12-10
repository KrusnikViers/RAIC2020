#pragma once

#include <set>
#include <vector>

#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  void update(const PlayerView& view);

  int id;
  int tick_number = 0;

  EntityList drones;
  EntityList melees;
  EntityList ranged;
  EntityList battle_units;

  EntityList buildings;

  EntityList enemies;
  EntityList resources;

  int resource;
  int supply_used;
  int supply_now;
  int supply_building;

  std::vector<std::vector<const Entity*>> map;
  std::unordered_map<EntityType, EntityProperties> props;
  EntityMap all;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();
