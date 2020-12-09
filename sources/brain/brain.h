#pragma once

#include <map>
#include <unordered_set>

#include "brain/building.h"
#include "brain/fighting.h"
#include "brain/state.h"
#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class Brain {
 public:
  Action update(const PlayerView& playerView, DebugInterface* debugInterface);

 private:
  int army_size_ = 18;

  State state_;
  BuildingPlanner building_;
  FightingPlanner fighting_;
};
