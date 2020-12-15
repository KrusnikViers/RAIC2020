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
  const Entity* getNearestEnemy(const Entity* unit);
  Vec2Int getLeastKnownPosition(const Entity* unit);
};
