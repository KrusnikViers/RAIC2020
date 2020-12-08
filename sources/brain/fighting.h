#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"

class FightingPlanner {
 public:
  void update(const PlayerView& view, State& state);
  EntityAction command(const Entity* entity);
};
