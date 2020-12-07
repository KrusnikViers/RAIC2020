#pragma once

#include <map>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class Brain {
 public:
  enum InternalType {
    EnemyUnit = 0,
    Resource,
  };

  Brain() = default;

  Action update(const PlayerView& playerView, DebugInterface* debugInterface);
  void debug(const PlayerView& playerView, DebugInterface& debugInterface);

  const Entity* getNearest(Vec2Int position, InternalType type,
                           bool unassigned);
  Vec2Int getNearestPlacing(Vec2Int position);
  void updateStored();

 private:
  int id_                 = -1;
  const PlayerView* view_ = nullptr;
  State state;

  int builder_id_ = -1;

  std::unordered_map<int, int> targeted_;
  std::unordered_map<int, int> assigned_;
};
