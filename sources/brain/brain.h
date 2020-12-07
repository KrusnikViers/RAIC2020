#pragma once

#include <map>
#include <unordered_set>

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

  const Entity* getNearest(Vec2Int position, InternalType type, bool unassigned);
  Vec2Int getNearestPlacing(Vec2Int position);
  void updateStored();
  void maybeInit();

 private:
  int id_                 = -1;
  const PlayerView* view_ = nullptr;

  int builder_id_ = -1;

  int builders_count_ = 0;
  int ranged_count_ = 0;
  int melee_count_ = 0;

  std::unordered_map<int, int> targeted_;
  std::unordered_map<int, int> assigned_;
  std::vector<std::vector<const Entity*>> map_;
};
