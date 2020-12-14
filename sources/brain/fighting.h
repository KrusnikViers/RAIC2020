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

  int needed_army     = 20;
  bool recovery       = true;
  bool critical_fight = false;

 private:
  void fillHeatMap();
  const Entity* getNearestEnemy(const Entity* unit, bool guard);
  Vec2Int getBestGuardPosition(const Entity* unit);

  std::unordered_set<int> guard_awaken_;
  std::unordered_map<int, Vec2Int> guard_posts_;

  std::unordered_map<int, const Entity*> attackers_;
  std::unordered_map<int, const Entity*> targeted_;
};
