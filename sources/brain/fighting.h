#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"

class FightingPlanner {
 public:
  void update(const PlayerView& view, State& state);
  EntityAction command(const Entity* entity);

  enum ThreatClass { Neutral, Approach, Threat, Attack };

 private:
  void fillHeatMap(State::EntityList list);
  const Entity* getNearestEnemy(const Entity* unit, bool guard);
  Vec2Int getBestGuardPosition(const Entity* unit);

  const State* state_;

  std::unordered_map<int, const Entity*> assault_;
  std::unordered_map<int, const Entity*> guard_;

  std::unordered_map<int, Vec2Int> guard_posts_;

  std::vector<std::vector<ThreatClass>> heatmap_;
  std::unordered_map<int, const Entity*> attackers_;
  std::unordered_map<int, const Entity*> targeted_;
};
