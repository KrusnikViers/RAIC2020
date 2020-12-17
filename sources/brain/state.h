#pragma once

#include <set>
#include <unordered_set>
#include <vector>

#include "brain/routing.h"
#include "brain/utils.h"
#include "model/Model.hpp"

class State {
 public:
  typedef std::vector<const Entity*> EntityList;
  typedef std::unordered_map<int, const Entity*> EntityMap;

  void update(const PlayerView& view);

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

 private:
  void maybeInit(const PlayerView& view);
  void updateEntities(const PlayerView& view);

  int player_id_;
  EntityList dummy_;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
State& state();

inline std::unordered_map<EntityType, EntityProperties>& props() {
  return state().props;
}
