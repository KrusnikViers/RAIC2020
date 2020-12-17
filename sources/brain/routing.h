#pragma once

#include <vector>

#include "brain/utils.h"
#include "model/Model.hpp"

enum CellFuturePurpose { NoPurpose, DronePosition, UnitPosition };
enum CellAttackStatus { Safe, Threat, Attack };

struct MapCell {
  EntityType last_seen_entity = NONE;
  const Entity* entity        = nullptr;

  CellFuturePurpose purpose = NoPurpose;
  CellAttackStatus attack_status;

  int blind_counter = -1;
};

struct RoutePoint {
  int distance = -1;
  Vec2Int first_step;
};

template <class T>
using map_t = std::vector<std::vector<T>>;

class Map {
 public:
  void update(const PlayerView& view);

  int size;
  MapCell& at(int x, int y) { return map_[x][y]; }

  const map_t<RoutePoint>& routes(const Entity* entity, bool rebuild = false);

 private:
  void resetCell(MapCell& cell) {
    cell.entity        = nullptr;
    cell.purpose       = NoPurpose;
    cell.attack_status = Safe;
  }

  void maybeInit(const PlayerView& view);
  void buildMap(map_t<RoutePoint>& layer, const Entity* entity);

  map_t<MapCell> map_;
  std::vector<map_t<RoutePoint>> maps_cache_;
  std::unordered_map<int, int> cache_index_for_entity_;
};

// Singleton implementation. For all the methods in planners called, you can
// assume that state represents refreshed status for the current tick.
Map& map();

inline MapCell& cell(int x, int y) { return map().at(x, y); }
inline MapCell& cell(Vec2Int pos) { return map().at(pos.x, pos.y); }
