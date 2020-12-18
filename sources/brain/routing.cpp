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
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      resetCell(cell(i, j));
      if (!((i + 4) % 6) && !((j + 4) % 6))
        ++cell(i, j).blind_counter;
      else
        cell(i, j).blind_counter = 1;
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
      if (entity->entityType != RANGED && entity->entityType != MELEE &&
          entity->entityType != TURRET)
        continue;
      const int attack_range = props()[entity->entityType].attack->attackRange;
      for (const auto& cell_pos :
           nearestCells(entity->position, attack_range + 3)) {
        map_[cell_pos.x][cell_pos.y].attack_status =
            dist(cell_pos, entity->position) <= attack_range ? Attack : Threat;
        if (map_[cell_pos.x][cell_pos.y].blind_counter)
          ++map_[cell_pos.x][cell_pos.y].blind_counter;
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

void Map::maybeInit(const PlayerView& view) {
  if (!map_.empty()) return;
  size = view.mapSize;

  map_.resize(size);
  for (auto& row : map_) row.resize(size);

  map_[2][78].blind_counter  = 10;
  map_[78][2].blind_counter  = 10;
  map_[78][78].blind_counter = 10;
}

std::shared_ptr<MoveAction> Map::moveAction(const Entity* entity,
                                            Vec2Int position) {
  return actionMove(position, true);
}

Vec2Int Map::leastKnownPosition() {
  std::pair<int, int> score(-1, 0);
  Vec2Int best_result(size / 2, size / 2);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      auto new_score =
          std::make_pair(cell(i, j).blind_counter, size - std::max(i, j));
      if (new_score > score) {
        score       = new_score;
        best_result = Vec2Int(i, j);
      }
    }
  }
  return best_result;
}
