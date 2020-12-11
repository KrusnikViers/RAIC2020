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

  int map_size;
  MapCell& cell(Vec2Int pos) { return map[pos.x][pos.y]; }
  MapCell& cell(int x, int y) { return map[x][y]; }

  int id;

  EntityList drones;
  EntityList melees;
  EntityList ranged;
  EntityList buildings;

  EntityList battle_units;

  EntityList enemies;
  EntityList resources;

  int initial_resource = -1;
  int resource;
  int supply_used;
  int supply_now;
  int supply_building;

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

  std::vector<std::vector<MapCell>> map;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();
