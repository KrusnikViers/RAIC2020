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
  struct Cell {
    bool move_away;
    bool taken_by;
  };

  struct Command {
    Command()                     = default;
    Command(const Command& other) = default;
    Command(Vec2Int p, EntityType t) : pos(p), type(t){};
    Command(Vec2Int p, Vec2Int p2, EntityType t)
        : pos(p), move_to(p2), type(t){};

    bool operator<(const Command& other) const { return type < other.type; }

    Vec2Int pos;
    Vec2Int move_to;
    EntityType type;
  };

  void repairBuildings(const State& state, const State::EntityList& list);
  void build(const State& state, EntityType type);
  void run();
  void dig();

  int nearestFreeDrone(Vec2Int position, const State& state) const;
  Vec2Int nearestFreePlacing(const State& state, EntityType type) const;
  Vec2Int nearestFreePlace(Vec2Int pos) const;
  std::vector<std::pair<Vec2Int, Vec2Int>> BuildingPlanner::diggingPlaces()
      const;

  const State* state_;
  std::vector<std::vector<Cell>> map_;
  std::unordered_map<int, Command> commands_;
};
