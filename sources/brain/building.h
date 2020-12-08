#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"

class BuildingPlanner {
 public:
  void update(const PlayerView& view, State& state);
  EntityAction command(const Entity* entity);

 private:
  struct Command {
    Vec2Int pos;
    EntityType type;
  };

  std::unordered_map<int, Command> commands_;
};
