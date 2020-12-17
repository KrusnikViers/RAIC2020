#include "brain/state.h"

#include <algorithm>
#include <queue>

namespace {

State instance;

struct PathNode {
  PathNode()                      = default;
  PathNode(const PathNode& other) = default;
  PathNode(const int score, int x, int y, int offset_x, int offset_y)
      : score(score), x(x), y(y), offset_x(offset_x), offset_y(offset_y) {}

  bool operator<(const PathNode& other) const {
    if (score != other.score) return score < other.score;
    if (x != other.x) return x < other.x;
    return y < other.y;
  }

  int score;
  int x, y;
  int offset_x, offset_y;
};

}  // namespace

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
  production_queue.clear();

  targeted.clear();
  planned_damage.clear();
  map_ids_.clear();

  for (auto& row : map_)
    for (auto& cell : row) ++cell.last_visible;

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

  // Give some bonus to the enemy bases location to check them first.
  map_[0].back().last_visible     = 100;
  map_.back()[0].last_visible     = 100;
  map_.back().back().last_visible = 50;
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

  const int attack_radius = props[RANGED].attack->attackRange;
  for (const auto& unit : my(RANGED)) {
    for (const auto& enemy_unit : enemies) {
      if (m_dist(enemy_unit->position, unit->position) <= attack_radius) {
        targeted[unit->id].push_back(enemy_unit);
      }
    }
  }
}

const map_t<RouteDirection>& State::mapFor(const Entity* entity) {
  if (map_ids_.count(entity->id)) return maps_cache_[map_ids_[entity->id]];

  const int current_id = (int)map_ids_.size();
  map_ids_[entity->id] = current_id;
  if (map_ids_.size() > maps_cache_.size()) {
    maps_cache_.emplace_back();
    maps_cache_.back().resize(map_size);
    for (auto& row : maps_cache_.back()) row.resize(map_size);
  }

  buildMap(maps_cache_[map_ids_[entity->id]], entity);
  return maps_cache_[map_ids_[entity->id]];
}

void State::buildMap(map_t<RouteDirection>& layer, const Entity* entity) {
  const int resource_weight = props[RESOURCE].maxHealth / 5;
  const int kInfinity       = 100000;
  for (auto& row : layer) {
    for (auto& cell : row) cell = RouteDirection();
  }
  layer[entity->position.x][entity->position.y] = {0, 0, 0};

  auto getNode = [&layer](int x, int y) {
    return PathNode(layer[x][y].distance, x, y,  //
                    layer[x][y].offset_x, layer[x][y].offset_y);
  };

  std::set<PathNode> nodes_to_see;
  nodes_to_see.insert(getNode(entity->position.x, entity->position.y));

  while (!nodes_to_see.empty()) {
    int score = nodes_to_see.begin()->score;
    int x     = nodes_to_see.begin()->x;
    int y     = nodes_to_see.begin()->y;
    Vec2Int node_offset(nodes_to_see.begin()->offset_x,
                        nodes_to_see.begin()->offset_y);
    nodes_to_see.erase(nodes_to_see.begin());

    for (const auto& offset :
         {Vec2Int(-1, 0), Vec2Int(1, 0), Vec2Int(0, -1), Vec2Int(0, 1)}) {
      Vec2Int n(x + offset.x, y + offset.y);
      if (isOut(n.x, n.y)) continue;
      int bonus = -1;
      if ((!cell(n).last_visible && !cell(n).entity) ||
          (cell(n).last_visible && cell(n).last_seen_entity != RESOURCE)) {
        bonus = 1;
      } else if (cell(n).last_seen_entity == RESOURCE) {
        bonus = resource_weight;
      }

      if (bonus == -1) continue;
      if (score + bonus < layer[n.x][n.y].distance) {
        if (layer[n.x][n.y].distance != kInfinity) {
          auto old_node = getNode(n.x, n.y);
          nodes_to_see.erase(old_node);
        }
        layer[n.x][n.y].distance = score + bonus;
        if (!node_offset.x && !node_offset.y) {
          layer[n.x][n.y].offset_x = offset.x;
          layer[n.x][n.y].offset_y = offset.y;
        } else {
          layer[n.x][n.y].offset_x = node_offset.x;
          layer[n.x][n.y].offset_y = node_offset.y;
        }
        nodes_to_see.insert(getNode(n.x, n.y));
      }
    }
  }
}
