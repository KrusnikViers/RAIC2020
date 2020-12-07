#pragma once

#include <vector>

#include "model/Model.hpp"
#include "strategy/DebugInterface.hpp"

class State {
 public:
  State() = default;

  void update(const PlayerView& view);

  struct ChangingCounter {
    int now     = 0;
    int planned = 0;
  };

  ChangingCounter builders;
  ChangingCounter melee;
  ChangingCounter ranged;
  ChangingCounter supply;
  int resource;

  const std::vector<std::vector<const Entity*>>& map() const { return map_; }
  const std::unordered_map<int, const Entity*>& entities() const {
    return entities_;
  }

 private:
  void fillMap(Vec2Int point, int size, const Entity* entity);

  std::vector<std::vector<const Entity*>> map_;
  std::unordered_map<int, const Entity*> entities_;
};