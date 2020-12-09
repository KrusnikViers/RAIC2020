#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"

class FightingPlanner {
 public:
  void update(const PlayerView& view, State& state);
  EntityAction command(const Entity* entity);
  Vec2Int whereToSpawn(const Entity* building);

  enum ThreatClass { Neutral, Protected };

  struct Cell {
    ThreatClass threat;
    bool guard_post = false;
  };

 private:
  void fillHeatMap();
  const Entity* getNearestEnemy(const Entity* unit, bool guard);
  Vec2Int getBestGuardPosition(const Entity* unit);

  const State* state_;

  std::unordered_set<int> guard_awaken_;
  std::unordered_map<int, Vec2Int> guard_posts_;

  std::vector<std::vector<Cell>> map_;
  std::unordered_map<int, const Entity*> attackers_;
  std::unordered_map<int, const Entity*> targeted_;
};
