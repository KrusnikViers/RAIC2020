#pragma once

#include <vector>

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
  EntityList bases;
  EntityList m_barracks;
  EntityList r_barracks;
  EntityList turrets;
  EntityList enemies;
  EntityList resources;

  int resource;
  int resource_planned;
  int supply_used;
  int supply_now;
  int supply_building;

  std::vector<std::vector<const Entity*>> map;
  EntityMap all;
};