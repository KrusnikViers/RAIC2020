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
  const Entity* getTargetedEnemy(const Entity* unit);
  const Entity* getNearestEnemy(const Entity* unit);
};
