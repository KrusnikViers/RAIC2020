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
  buffered_entity_ = nullptr;

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
      // cell(entity->position).future_unit_placement = true;
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

const map_t<RoutePoint>& Map::routes(const Entity* entity) {
  if (buffered_entity_ == entity) return map_buffer_;
  buffered_entity_ = entity;

  if (map_buffer_.empty()) {
    map_buffer_.resize(size);
    for (auto& row : map_buffer_) row.resize(size);
  }

  buildMap(map_buffer_, entity);
  return map_buffer_;
}

void Map::maybeInit(const PlayerView& view) {
  if (!map_.empty()) return;
  size = view.mapSize;

  map_.resize(size);
  for (auto& row : map_) row.resize(size);
}

void Map::buildMap(map_t<RoutePoint>& layer, const Entity* entity) {
  for (auto& row : layer) {
    for (auto& cell : row) cell = RoutePoint();
  }

  std::queue<Vec2Int> nodes_to_see;
  layer[entity->position.x][entity->position.y] = {0, Vec2Int(0, 0)};
  nodes_to_see.push(entity->position);

  std::vector<Vec2Int> offsets = {Vec2Int(-1, 0), Vec2Int(1, 0), Vec2Int(0, -1),
                                  Vec2Int(0, 1)};
  std::shuffle(offsets.begin(), offsets.end(), rand_gen_);

  while (!nodes_to_see.empty()) {
    const auto current         = nodes_to_see.front();
    const int current_distance = layer[current.x][current.y].distance;
    nodes_to_see.pop();

    for (const auto& offset : offsets) {
      Vec2Int new_step(current.x + offset.x, current.y + offset.y);
      if (isOut(new_step.x, new_step.y)) continue;

      auto& layer_cell     = layer[new_step.x][new_step.y];
      const auto& new_cell = cell(new_step);
      int new_distance     = 1;
      if (new_cell.last_seen_entity == RESOURCE)
        new_distance = 6;
      else if (new_cell.last_seen_entity != NONE &&
               props()[new_cell.last_seen_entity].canMove)
        new_distance = 80;
      else if (new_cell.future_unit_placement)
        new_distance = 160;
      if (layer_cell.distance != -1 &&
          layer_cell.distance <= current_distance + new_distance)
        continue;

      bool can_move_through =
          new_cell.last_seen_entity == NONE || new_distance != 1;
      if (!can_move_through) continue;

      layer_cell.distance   = current_distance + new_distance;
      layer_cell.first_step = layer[current.x][current.y].first_step;
      if (!layer_cell.first_step.x && !layer_cell.first_step.y)
        layer_cell.first_step = offset;
      nodes_to_see.push(new_step);
    }
  }
}

std::shared_ptr<MoveAction> Map::moveAction(const Entity* entity,
                                            Vec2Int position) {
  Vec2Int new_position = entity->position;
  if (position.x != entity->position.x || position.y != entity->position.y) {
    const auto& routing_map = routes(entity);
    new_position            = posOffset(entity->position,
                             routing_map[position.x][position.y].first_step);
  }
  const auto& next_cell = cell(new_position);

  // cell(entity->position).future_unit_placement = false;
  cell(new_position).future_unit_placement = true;
  return (new_position.x == entity->position.x &&
          new_position.y == entity->position.y)
             ? nullptr
             : std::make_shared<MoveAction>(new_position, false, true);
}

Vec2Int Map::leastKnownPosition() {
  int score = -1;
  Vec2Int best_result(size / 2, size / 2);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      const int new_score = cell(i, j).blind_counter + (size - std::max(i, j));
      if (cell(i, j).blind_counter && (score == -1 || new_score >= score)) {
        score       = new_score;
        best_result = Vec2Int(i, j);
      }
    }
  }
  return best_result;
}
