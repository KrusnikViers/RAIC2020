#include "brain/routing.h"

#include <queue>

#include "brain/state.h"

namespace {

Map g_map;

}

Map& map() { return g_map; }

void Map::update(const PlayerView& view) {
  maybeInit(view);

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
      const int attack_range = props()[entity->entityType].attack->attackRange;
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
                                     bool ignore_resources) {
  if (map_buffer_.empty()) {
    map_buffer_.resize(size);
    for (auto& row : map_buffer_) row.resize(size);
  }
  buildMap(map_buffer_, entity, ignore_resources);
  return map_buffer_;
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

void Map::buildMap(map_t<RoutePoint>& layer, const Entity* entity,
                   bool ignore_resources) {
  for (auto& row : layer) {
    for (auto& cell : row) cell = RoutePoint();
  }

  std::queue<Vec2Int> nodes_to_see;
  layer[entity->position.x][entity->position.y] = {0, Vec2Int(0, 0)};
  nodes_to_see.push(entity->position);

  while (!nodes_to_see.empty()) {
    const auto current = nodes_to_see.front();
    nodes_to_see.pop();

    for (const auto& offset :
         {Vec2Int(-1, 0), Vec2Int(1, 0), Vec2Int(0, -1), Vec2Int(0, 1)}) {
      Vec2Int new_step(current.x + offset.x, current.y + offset.y);
      if (isOut(new_step.x, new_step.y)) continue;

      auto& layer_cell = layer[new_step.x][new_step.y];
      if (layer_cell.distance != -1) continue;

      const auto& new_cell = cell(new_step);
      const bool empty_and_visible =
          !new_cell.blind_counter &&
          (!new_cell.entity || props()[new_cell.entity->entityType].canMove);
      if (!empty_and_visible &&
          !(new_cell.last_seen_entity == RESOURCE && ignore_resources)) {
        continue;
      }

      layer_cell.distance   = layer[current.x][current.y].distance + 1;
      layer_cell.first_step = layer[current.x][current.y].first_step;
      if (!layer_cell.first_step.x && !layer_cell.first_step.y)
        layer_cell.first_step = offset;
      nodes_to_see.push(new_step);
    }
  }
}

std::shared_ptr<MoveAction> Map::moveAction(const Entity* entity,
                                            Vec2Int position) {
  Vec2Int new_offset;
  const auto& routing_map = routes(entity);
  if (routing_map[position.x][position.y].distance != -1) {
    new_offset = routing_map[position.x][position.y].first_step;
  } else {
    const auto& breakthrough_map = routes(entity, true);
    new_offset = breakthrough_map[position.x][position.y].first_step;
  }

  if (!new_offset.x && !new_offset.y) {
    return std::make_shared<MoveAction>(position, true, true);
  }
  return std::make_shared<MoveAction>(
      Vec2Int(entity->position.x + new_offset.x,
              entity->position.y + new_offset.y),
      true, true);
}

Vec2Int Map::leastKnownPosition() {
  int score = -1;
  Vec2Int best_result(size / 2, size / 2);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      const int blind_counter = cell(i, j).blind_counter;
      if (blind_counter && (score == -1 || blind_counter >= score)) {
        score       = blind_counter;
        best_result = Vec2Int(i, j);
      }
    }
  }
  return best_result;
}
