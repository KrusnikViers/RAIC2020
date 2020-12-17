#include "brain/state.h"

#include <algorithm>
#include <queue>

namespace {

State instance;

}  // namespace

State& state() { return instance; }

void State::update(const PlayerView& view) {
  maybeInit(view);

  for (const auto& player : view.players) {
    if (player.id == player_id_) resource = player.resource;
  }

  all.clear();
  my_units.clear();
  enemies.clear();
  resources.clear();
  supply_used = supply_now = supply_building = visible_resource = 0;
  production_queue.clear();

  targeted.clear();
  planned_damage.clear();

  updateEntities(view);

  map().update(view);
}

void State::maybeInit(const PlayerView& view) {
  // Prepare containers
  if (!props.empty()) return;

  player_id_ = view.myId;
  props      = view.entityProperties;
}

void State::updateEntities(const PlayerView& view) {
  for (const auto& entity : view.entities) {
    all[entity.id] = &entity;

    if (entity.entityType == RESOURCE) {
      // Resource
      resources.push_back(&entity);
      visible_resource += entity.health;
    } else if (*entity.playerId != player_id_) {
      // Enemy
      enemies.push_back(&entity);
    } else {
      // My units
      my_units[entity.entityType].push_back(&entity);
      supply_used += props.at(entity.entityType).populationUse;
      (entity.active ? supply_now : supply_building) +=
          props.at(entity.entityType).populationProvide;
    }
  }

  const int attack_radius = props[RANGED].attack->attackRange;
  for (const auto& unit : my(RANGED)) {
    for (const auto& enemy_unit : enemies) {
      if (dist(enemy_unit->position, unit->position) <= attack_radius) {
        targeted[unit->id].push_back(enemy_unit);
      }
    }
  }
}
