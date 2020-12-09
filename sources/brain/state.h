#pragma once

#include <vector>
#include <set>

#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  void update(const PlayerView& view);

  int id;
  EntityList drones;
  EntityList melees;
  EntityList ranged;
  EntityList supplies;
  EntityList buildings;
  EntityList enemies;
  EntityList resources;
  EntityList battle_units;

  std::set<EntityType> to_build;

  int resource;
  int resource_planned;
  int supply_used;
  int supply_now;
  int supply_building;

  std::vector<std::vector<const Entity*>> map;
  std::unordered_map<EntityType, EntityProperties> props;
  EntityMap all;
};