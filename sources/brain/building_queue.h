#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "model/Model.hpp"

class BuildingQueue {
 public:
  BuildingQueue() = default;

  struct EnqueuedStructure {
    Vec2Int pos;
    EntityType type;
    int building_id = -1;
  };

  void update(const PlayerView& view, const State& state);
  void enqueue(const PlayerView& view, const State& state, EntityType type);
  EntityAction commandsForBuilder(const Entity* entity);

 private:
  std::unordered_set<int> being_digged_;
  std::unordered_map<int, Vec2Int> digging_;
  std::shared_ptr<EnqueuedStructure> waiting_;
  std::unordered_map<int, EnqueuedStructure> building_;
};
