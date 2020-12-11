#pragma once

#include <set>
#include <vector>

#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  enum CellProtectionClass { Neutral, Protected };

  struct MapCell {
    const Entity* entity = nullptr;
    CellProtectionClass protection_class;
    bool guard_planned_position;
    bool drone_planned_position;
    bool drone_danger_area;
  };

  void update(const PlayerView& view);

  bool has(EntityType type) const;

  int id;
  int tick_number = 0;
  int map_size;

  EntityList drones;
  EntityList melees;
  EntityList ranged;
  EntityList battle_units;

  EntityList buildings;

  EntityList enemies;
  EntityList resources;

  int initial_resource = -1;
  int resource;
  int supply_used;
  int supply_now;
  int supply_building;

  std::vector<std::vector<MapCell>> map;
  std::unordered_map<EntityType, EntityProperties> props;
  EntityMap all;

 private:
  void resetCell(MapCell& cell) {
    cell.entity                 = nullptr;
    cell.protection_class       = Neutral;
    cell.guard_planned_position = false;
    cell.drone_planned_position = false;
    cell.drone_danger_area      = false;
  }
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();
