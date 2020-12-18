#pragma once

#include <vector>

#include "brain/utils.h"
#include "model/Model.hpp"

enum CellAttackStatus { Safe, Threat, Attack };

struct MapCell {
  EntityType last_seen_entity = NONE;
  const Entity* entity        = nullptr;

  bool position_taken = false;
  CellAttackStatus attack_status;

  int blind_counter = -1;
};

template <class T>
using map_t = std::vector<std::vector<T>>;

class Map {
 public:
  void update(const PlayerView& view);

  int size;
  MapCell& at(int x, int y) { return map_[x][y]; }

  std::shared_ptr<MoveAction> moveAction(const Entity* entity,
                                         Vec2Int position);
  Vec2Int leastKnownPosition();

 private:
  void resetCell(MapCell& cell) {
    cell.entity         = nullptr;
    cell.position_taken = false;
    cell.attack_status  = Safe;
  }

  void maybeInit(const PlayerView& view);

  map_t<MapCell> map_;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
Map& map();

inline MapCell& cell(int x, int y) { return map().at(x, y); }
inline MapCell& cell(Vec2Int pos) { return map().at(pos.x, pos.y); }
