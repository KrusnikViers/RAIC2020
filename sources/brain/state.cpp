#include "brain/state.h"

#include <algorithm>

namespace {

State instance;

}

State& state() { return instance; }

void State::update(const PlayerView& view) {
  maybeInit(view);

  for (auto& row : map_)
    for (auto& cell : row) resetCell(cell);
  for (const auto& player : view.players) {
    if (player.id == player_id_) resource = player.resource;
  }

  all.clear();
  my_units.clear();
  enemies.clear();
  resources.clear();
  supply_used = supply_now = supply_building = visible_resource = 0;
  supply_required = barracks_required = false;

  updateEntities(view);

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

void State::maybeInit(const PlayerView& view) {
  // Prepare containers
  if (!map_.empty()) return;

  player_id_ = view.myId;
  props      = view.entityProperties;
  map_size   = view.mapSize;

  map_.resize(map_size);
  for (auto& row : map_) row.resize(map_size);
}

void State::updateEntities(const PlayerView& view) {
  for (const auto& entity : view.entities) {
    all[entity.id] = &entity;

    const int entity_size = props.at(entity.entityType).size;
    for (int i = 0; i < entity_size; ++i) {
      for (int j = 0; j < entity_size; ++j) {
        map_[entity.position.x + i][entity.position.y + j].entity = &entity;
      }
    }

    if (entity.entityType == RESOURCE) {
      // Resource
      resources.push_back(&entity);
      visible_resource += entity.health;

    } else if (*entity.playerId != player_id_) {
      // Enemy
      enemies.push_back(&entity);
      if (!props[entity.entityType].attack) continue;
      const int attack_zone = props[entity.entityType].attack->attackRange;
      for (const auto& cell_pos :
           nearestCells(entity.position, attack_zone + 2)) {
        cell(cell_pos).attack_status =
            m_dist(cell_pos, entity.position) <= attack_zone ? Attack : Threat;
      }

    } else {
      // My units
      my_units[entity.entityType].push_back(&entity);
      supply_used += props.at(entity.entityType).populationUse;
      (entity.active ? supply_now : supply_building) +=
          props.at(entity.entityType).populationProvide;

      const int visibility_radius = props.at(entity.entityType).sightRange;
      for (int i = 0; i < entity_size; ++i) {
        for (int j = 0; j < entity_size; ++j) {
          for (const auto& cell :
               nearestCells(entity.position.x + i, entity.position.y + j,
                            visibility_radius)) {
            map_[cell.x][cell.y].last_visible = 0;
          }
        }
      }
    }
  }
}
