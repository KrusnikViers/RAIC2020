#include "brain/state.h"

void State::update(const PlayerView& view) {
  // Prepare containers
  if (map_.empty()) {
    map_.resize(view.mapSize);
    for (auto& row : map_) row.resize(view.mapSize);
  }

  // Reset storage
  for (auto& row : map_)
    for (auto& cell : row) cell = nullptr;
  entities_.clear();
  builders.now = builders.planned = 0;
  melee.now = melee.planned = 0;
  ranged.now = ranged.planned = 0;
  supply.now = supply.planned = 0;

  // Refill storage
  const int my_id = view.myId;

  for (const auto& player : view.players) {
    if (player.id == my_id) {
      resource = player.resource;
      break;
    }
  }

  for (const auto& entity : view.entities) {
    entities_[entity.id] = &entity;
    fillMap(entity.position, view.entityProperties.at(entity.entityType).size,
            &entity);
    if (!entity.playerId || *entity.playerId != my_id) continue;
    switch (entity.entityType) {
      case BUILDER_BASE:
      case MELEE_BASE:
      case RANGED_BASE:
      case HOUSE:
        (entity.active ? supply.now : supply.planned) +=
            view.entityProperties.at(entity.entityType).populationProvide;
        break;

      case BUILDER_UNIT:
        ++builders.now;
        break;

      case MELEE_UNIT:
        ++melee.now;
        break;

      case RANGED_UNIT:
        ++ranged.now;
        break;
    }
  }
}

void State::fillMap(Vec2Int point, int size, const Entity* entity) {
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) map_[point.x + i][point.y + j] = entity;
  }
}
