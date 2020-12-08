#include "brain/state.h"

#include "model/DebugCommand.hpp"

void State::update(const PlayerView& view) {
  // Prepare containers
  if (map.empty()) {
    map.resize(view.mapSize);
    for (auto& row : map) row.resize(view.mapSize);
  }
  if (props.empty()) props = view.entityProperties;

  // Reset storage
  id = view.myId;
  for (auto& row : map)
    for (auto& cell : row) cell = nullptr;

  all.clear();
  drones.clear();
  melees.clear();
  ranged.clear();
  supplies.clear();
  bases.clear();
  m_barracks.clear();
  r_barracks.clear();
  turrets.clear();
  enemies.clear();
  resources.clear();

  resource = resource_planned = supply_used = supply_now = supply_building = 0;

  // Refill storage
  for (const auto& player : view.players) {
    if (player.id == id) {
      resource = player.resource;
      break;
    }
  }

  for (const auto& entity : view.entities) {
    all[entity.id] = &entity;

    const int entity_size = props.at(entity.entityType).size;
    for (int i = 0; i < entity_size; ++i) {
      for (int j = 0; j < entity_size; ++j)
        map[entity.position.x + i][entity.position.y + j] = &entity;
    }

    if (entity.entityType == RESOURCE) {
      resources.push_back(&entity);
    } else if (*entity.playerId != id) {
      enemies.push_back(&entity);
    } else {
      switch (entity.entityType) {
        case BUILDER_UNIT:
          drones.push_back(&entity);
          break;
        case MELEE_UNIT:
          melees.push_back(&entity);
          break;
        case RANGED_UNIT:
          ranged.push_back(&entity);
          break;
        case MELEE_BASE:
          m_barracks.push_back(&entity);
          break;
        case RANGED_BASE:
          r_barracks.push_back(&entity);
          break;
        case BUILDER_BASE:
          bases.push_back(&entity);
          break;
        case HOUSE:
          supplies.push_back(&entity);
          break;
        case TURRET:
          turrets.push_back(&entity);
          break;
      }

      supply_used += props.at(entity.entityType).populationUse;
      (entity.active ? supply_now : supply_building) +=
          props.at(entity.entityType).populationProvide;
    }
  }
}
