#pragma once

#include <set>
#include <vector>

#include "brain/utils.h"
#include "model/Model.hpp"

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  void update(const PlayerView& view);

  // Map-related stuff
  enum CellFuturePurpose { None, Drone, Guard, Unit };
  enum CellProtectionClass { Neutral, Protected };

  struct MapCell {
    CellFuturePurpose purpose   = None;
    EntityType last_seen_entity = NONE;
    const Entity* entity        = nullptr;

    CellProtectionClass protection_class;
    bool guard_planned_position;
    bool drone_planned_position;
    bool drone_danger_area;

    int last_visible = -1;
  };

  int map_size;
  MapCell& cell(Vec2Int pos) { return map_[pos.x][pos.y]; }
  MapCell& cell(int x, int y) { return map_[x][y]; }

  // Player-related stuff
  struct OtherPlayer {
    int id;
    int score;
    int resource;
    std::unordered_map<EntityType, EntityList> units;
  };

  std::unordered_map<EntityType, EntityList> my_units;
  std::vector<OtherPlayer> enemies;
  EntityList resources;
  bool has(EntityType type) const {
    return my_units.count(type) && !my_units.at(type).empty();
  }
  EntityList& my(EntityType type) {
    return my_units.count(type) ? my_units.at(type) : EntityList();
  }
  bool mine(const Entity* entity) const {
    return entity->playerId && *entity->playerId == player_id_;
  }

  // My player stuff.
  int resource;
  int visible_resource;
  int supply_used;
  int supply_now;
  int supply_building;

  std::unordered_map<EntityType, EntityProperties> props;
  std::unordered_map<int, const Entity*> all;

 private:
  void resetCell(MapCell& cell) {
    cell.entity                 = nullptr;
    cell.purpose                = None;
    cell.protection_class       = Neutral;
    cell.guard_planned_position = false;
    cell.drone_planned_position = false;
    cell.drone_danger_area      = false;
    ++cell.last_visible;
  }

  int player_id_;
  std::unordered_map<int, size_t> player_index_;
  std::vector<std::vector<MapCell>> map_;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();
