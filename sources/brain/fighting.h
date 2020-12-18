#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "brain/utils.h"
#include "model/Model.hpp"

class FightingPlanner {
 public:
  void update();
  EntityAction command(const Entity* entity);

 private:
  enum TacticsDecision { Advance, Hold, Retreat };

  TacticsDecision estimate(const Entity* unit, Vec2Int* next_position);
  const Entity* getTargetedEnemy(const Entity* unit);
  const Entity* getNearestEnemy(const Entity* unit);
};
