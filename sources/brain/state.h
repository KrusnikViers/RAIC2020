#pragma once

#include <set>
#include <unordered_set>
#include <vector>

#include "brain/utils.h"
#include "model/Model.hpp"

enum CellFuturePurpose { NoPurpose, DronePosition, UnitPosition };
enum CellAttackStatus { Safe, Threat, Attack };

struct RouteDirection {
  int distance    = 100000;
  int8_t offset_x = 0;
  int8_t offset_y = 0;
};

template <class T>
using map_t = std::vector<std::vector<T>>;

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  void update(const PlayerView& view);

  // Map-related stuff
  struct MapCell {
    EntityType last_seen_entity = NONE;
    const Entity* entity        = nullptr;

    CellFuturePurpose purpose = NoPurpose;
    CellAttackStatus attack_status;

    int last_visible = -1;
  };

  int map_size;
  MapCell& cell(Vec2Int pos) { return map_[pos.x][pos.y]; }
  MapCell& cell(int x, int y) { return map_[x][y]; }

  // Player-related stuff
  std::unordered_map<EntityType, EntityList> my_units;
  EntityList enemies;
  EntityList resources;
  bool has(EntityType type) const {
    return my_units.count(type) && !my_units.at(type).empty();
  }
  const EntityList& my(EntityType type) {
    return my_units.count(type) ? my_units.at(type) : dummy_;
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

  std::unordered_set<EntityType> production_queue;

  std::unordered_map<EntityType, EntityProperties> props;
  std::unordered_map<int, const Entity*> all;

  std::unordered_map<int, std::vector<const Entity*>> targeted;
  std::unordered_map<int, int> planned_damage;

  const map_t<RouteDirection>& mapFor(const Entity* entity);

 private:
  void resetCell(MapCell& cell) {
    cell.entity        = nullptr;
    cell.purpose       = NoPurpose;
    cell.attack_status = Safe;
    ++cell.last_visible;
  }

  void maybeInit(const PlayerView& view);
  void updateEntities(const PlayerView& view);
  void buildMap(map_t<RouteDirection>& layer, const Entity* entity);

  int player_id_;
  map_t<MapCell> map_;
  std::vector<map_t<RouteDirection>> maps_cache_;
  std::unordered_map<int, int> map_ids_;
  EntityList dummy_;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();
