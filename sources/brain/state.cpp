#include "brain/state.h"

#include <algorithm>

namespace {

State instance;

}

State& state() { return instance; }

void State::update(const PlayerView& view) {
  const auto player_id_ = view.myId;

  // Prepare containers
  if (map_.empty()) {
    props    = view.entityProperties;
    map_size = view.mapSize;
    for (const auto& player : view.players) {
      if (player.id == player_id_) continue;
      enemies.emplace_back();
      enemies.back().id        = player.id;
      player_index_[player.id] = enemies.size() - 1;
    }

    map_.resize(map_size);
    for (auto& row : map_) row.resize(map_size);
  }

  for (auto& row : map_)
    for (auto& cell : row) resetCell(cell);
  for (const auto& player : view.players) {
    if (player.id == player_id_) {
      resource = player.resource;
    } else {
      enemies[player_index_[player.id]].resource = player.resource;
      enemies[player_index_[player.id]].score    = player.score;
      enemies[player_index_[player.id]].units.clear();
    }
  }

  all.clear();
  my_units.clear();
  enemies.clear();
  resources.clear();
  supply_used = supply_now = supply_building = visible_resource = 0;
  for (const auto& entity : view.entities) {
    all[entity.id] = &entity;

    const int entity_size       = props.at(entity.entityType).size;
    const int visibility_radius = props.at(entity.entityType).sightRange;
    for (int i = 0; i < entity_size; ++i) {
      for (int j = 0; j < entity_size; ++j) {
        map_[entity.position.x + i][entity.position.y + j].entity = &entity;
        for (const auto& cell :
             nearestCells(entity.position.x + i, entity.position.y + j,
                          visibility_radius)) {
          map_[cell.x][cell.y].last_visible = 0;
        }
      }
    }

    if (entity.entityType == RESOURCE) {
      resources.push_back(&entity);
      visible_resource += entity.health;
    } else if (*entity.playerId != player_id_) {
      enemies[*entity.playerId].units[entity.entityType].push_back(&entity);
    } else {
      my_units[entity.entityType].push_back(&entity);

      supply_used += props.at(entity.entityType).populationUse;
      (entity.active ? supply_now : supply_building) +=
          props.at(entity.entityType).populationProvide;
    }
  }

  for (auto& row : map_) {
    for (auto& cell : row) {
      if (!cell.last_visible) {
        if (!cell.entity)
          cell.last_seen_entity = NONE;
        else
          cell.last_seen_entity = cell.entity->entityType;
      }
    }
  }
}
