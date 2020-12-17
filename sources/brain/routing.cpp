#include "brain/routing.h"

#include "brain/state.h"

namespace {

Map g_map;

}

Map& map() { return g_map; }

void Map::update(const PlayerView& view) {
  maybeInit(view);

  cache_index_for_entity_.clear();

  // Reset map and increase blind counters
  for (auto& row : map_) {
    for (auto& cell : row) {
      resetCell(cell);
      ++cell.blind_counter;
    }
  }

  // Entities processing
  for (const auto entity_pair : state().all) {
    // Entity pointers on map
    const Entity* entity  = entity_pair.second;
    const int entity_size = props().at(entity->entityType).size;
    for (int i = 0; i < entity_size; ++i) {
      for (int j = 0; j < entity_size; ++j) {
        map_[entity->position.x + i][entity->position.y + j].entity = entity;
      }
    }

    if (!entity->playerId) continue;

    if (state().mine(entity)) {
      // Visibility status
      const int sight_range = props().at(entity->entityType).sightRange;
      for (int i = 0; i < entity_size; ++i) {
        for (int j = 0; j < entity_size; ++j) {
          for (const auto& cell :
               nearestCells(entity->position.x + i, entity->position.y + j,
                            sight_range)) {
            map_[cell.x][cell.y].blind_counter = 0;
          }
        }
      }
    } else {
      // Attack status
      if (!props().at(entity->entityType).attack) continue;
      const int attack_range =
          props()[entity->entityType].attack->attackRange;
      for (const auto& cell_pos :
           nearestCells(entity->position, attack_range + 2)) {
        map_[cell_pos.x][cell_pos.y].attack_status =
            dist(cell_pos, entity->position) <= attack_range ? Attack : Threat;
      }
    }
  }

  // Last seen porocessing
  for (auto& row : map_) {
    for (auto& cell : row) {
      if (!cell.blind_counter) {
        if (!cell.entity)
          cell.last_seen_entity = NONE;
        else
          cell.last_seen_entity = cell.entity->entityType;
      }
    }
  }
}

const map_t<RoutePoint>& Map::routes(const Entity* entity,
                                     bool rebuild) {
  //  if (map_ids_.count(entity->id)) return maps_cache_[map_ids_[entity->id]];

  // const int current_id = (int)map_ids_.size();
  // map_ids_[entity->id] = current_id;
  // if (map_ids_.size() > maps_cache_.size()) {
  //  maps_cache_.emplace_back();
  //  maps_cache_.back().resize(map_size);
  //  for (auto& row : maps_cache_.back()) row.resize(map_size);
  //}

  // buildMap(maps_cache_[map_ids_[entity->id]], entity);
  // return maps_cache_[map_ids_[entity->id]];
  return maps_cache_[0];
}

void Map::maybeInit(const PlayerView& view) {
  if (!map_.empty()) return;
  size = view.mapSize;

  map_.resize(size);
  for (auto& row : map_) row.resize(size);

  // Give some bonus to the enemy bases locations to check them first.
  map_[0].back().blind_counter     = 100;
  map_.back()[0].blind_counter     = 100;
  map_.back().back().blind_counter = 50;
}

void Map::buildMap(map_t<RoutePoint>& layer, const Entity* entity) {
  //{
  //  const int resource_weight = props[RESOURCE].maxHealth / 5;
  //  const int kInfinity       = 100000;
  //  for (auto& row : layer) {
  //    for (auto& cell : row) cell = RouteDirection();
  //  }
  //  layer[entity->position.x][entity->position.y] = {0, 0, 0};

  //  auto getNode = [&layer](int x, int y) {
  //    return PathNode(layer[x][y].distance, x, y,  //
  //                    layer[x][y].offset_x, layer[x][y].offset_y);
  //  };

  //  std::set<PathNode> nodes_to_see;
  //  nodes_to_see.insert(getNode(entity->position.x, entity->position.y));

  //  while (!nodes_to_see.empty()) {
  //    int score = nodes_to_see.begin()->score;
  //    int x     = nodes_to_see.begin()->x;
  //    int y     = nodes_to_see.begin()->y;
  //    Vec2Int node_offset(nodes_to_see.begin()->offset_x,
  //                        nodes_to_see.begin()->offset_y);
  //    nodes_to_see.erase(nodes_to_see.begin());

  //    for (const auto& offset :
  //         {Vec2Int(-1, 0), Vec2Int(1, 0), Vec2Int(0, -1), Vec2Int(0, 1)}) {
  //      Vec2Int n(x + offset.x, y + offset.y);
  //      if (isOut(n.x, n.y)) continue;
  //      int bonus = -1;
  //      if ((!cell(n).blind_counter && !cell(n).entity) ||
  //          (cell(n).blind_counter && cell(n).last_seen_entity != RESOURCE)) {
  //        bonus = 1;
  //      } else if (cell(n).last_seen_entity == RESOURCE) {
  //        bonus = resource_weight;
  //      }

  //      if (bonus == -1) continue;
  //      if (score + bonus < layer[n.x][n.y].distance) {
  //        if (layer[n.x][n.y].distance != kInfinity) {
  //          auto old_node = getNode(n.x, n.y);
  //          nodes_to_see.erase(old_node);
  //        }
  //        layer[n.x][n.y].distance = score + bonus;
  //        if (!node_offset.x && !node_offset.y) {
  //          layer[n.x][n.y].offset_x = offset.x;
  //          layer[n.x][n.y].offset_y = offset.y;
  //        } else {
  //          layer[n.x][n.y].offset_x = node_offset.x;
  //          layer[n.x][n.y].offset_y = node_offset.y;
  //        }
  //        nodes_to_see.insert(getNode(n.x, n.y));
  //      }
  //    }
  //  }
}
