#pragma once

#include <map>
#include <unordered_set>

#include "brain/building_queue.h"
#include "brain/state.h"
#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class Brain {
 public:
  Action update(const PlayerView& playerView, DebugInterface* debugInterface);
  const Entity* getNearestEnemy(Vec2Int position);

 private:
  int id_ = -1;

  const PlayerView* view_ = nullptr;

  State state_;
  BuildingQueue building_queue_;
};
