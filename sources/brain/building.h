#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "brain/utils-inl.h"
#include "model/Model.hpp"

class BuildingPlanner {
 public:
  void update(const PlayerView& view, State& state);
  EntityAction command(const State& state, const Entity* entity);

 private:
  struct Command {
    Command()                     = default;
    Command(const Command& other) = default;
    Command(Vec2Int p, EntityType t) : pos(p), type(t){};

    Vec2Int pos;
    EntityType type;
  };

  void repairBuildings(const State& state, const State::EntityList& list);
  void build(const State& state, EntityType type);
  void dig(const State& state);

  int nearestFreeDrone(Vec2Int position, const State& state) const;
  Vec2Int nearestFreePlacing(const State& state, EntityType type) const;
  Vec2Int nearestFreeResource(const State& state,
                              const std::unordered_set<int>& taken_resources,
                              Vec2Int pos) const;

  std::unordered_map<int, Command> commands_;
  std::unordered_map<int, Vec2Int> move_away_;
};
