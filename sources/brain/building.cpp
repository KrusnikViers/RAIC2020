#include "brain/building.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>

namespace {

bool isOut(const State& state, int x, int y) {
  return x < 0 || y < 0 || x >= state.map.size() || y >= state.map.size();
}

bool isFree(const State& state, const Vec2Int& pos, bool or_drone = true) {
  if (isOut(state, pos.x, pos.y)) return false;
  const Entity* entity = state.map[pos.x][pos.y];
  return entity == nullptr || (or_drone && entity->entityType == BUILDER_UNIT &&
                               *entity->playerId == state.id);
}

}  // namespace

void BuildingPlanner::update(const PlayerView& view, State& state) {
  state_ = &state;
  commands_.clear();

  if (map_.empty()) {
    map_.resize(state.map.size());
    for (auto& row : map_) row.resize(state.map.size());
  }
  for (auto& row : map_) {
    for (auto& cell : row) {
      cell.move_away = false;
      cell.taken_by  = false;
    }
  }

  repair(state.buildings);

  if (state.supply_used >= (state.supply_now + state.supply_building) * 0.75 &&
      state.resource >= 100) {
    build(HOUSE);
  }

  run();
  dig();
}

EntityAction BuildingPlanner::command(const Entity* entity) {
  if (commands_.count(entity->id)) {
    const Command& command = commands_[entity->id];
    const int size         = state_->props.at(command.type).size;
    Vec2Int center(command.pos.x + size / 2, command.pos.y + size / 2);
    const Entity* target = state_->map[command.pos.x][command.pos.y];

    if (command.type == RESOURCE) {
      return EntityAction(
          std::make_shared<MoveAction>(command.move_to, true, true), nullptr,
          std::make_shared<AttackAction>(std::make_shared<int>(target->id),
                                         nullptr),
          nullptr);
    } else if (command.type == BUILDER_UNIT) {
      return EntityAction(
          std::make_shared<MoveAction>(command.move_to, true, true), nullptr,
          nullptr, nullptr);
    } else if (!target || target->entityType != command.type) {
      return EntityAction(
          std::make_shared<MoveAction>(center, true, true),
          std::make_shared<BuildAction>(command.type, command.pos), nullptr,
          nullptr);
    } else {
      return EntityAction(std::make_shared<MoveAction>(center, true, true),
                          nullptr, nullptr,
                          std::make_shared<RepairAction>(target->id));
    }
  }

  return EntityAction(nullptr, nullptr, nullptr, nullptr);
}

void BuildingPlanner::repair(const State::EntityList& list) {
  for (const auto& building : list) {
    if (building->health < state_->props.at(building->entityType).maxHealth) {
      int drone_id = nearestFreeDrone(building->position);
      if (drone_id == -1) continue;
      commands_[drone_id] = Command(building->position, building->entityType);
      map_[state_->all.at(drone_id)->position.x]
          [state_->all.at(drone_id)->position.y]
              .taken_by = true;
    }
  }
}

void BuildingPlanner::build(EntityType type) {
  Vec2Int best_place = nearestFreePlacing(type);
  if (best_place.x == -1) return;

  const int size = state_->props.at(type).size;
  Vec2Int center(best_place.x + size / 2, best_place.y + size / 2);

  const int drone_id = nearestFreeDrone(center);
  if (drone_id == -1) return;

  commands_[drone_id] = Command(best_place, type);
  map_[state_->all.at(drone_id)->position.x]
      [state_->all.at(drone_id)->position.y]
          .taken_by = true;
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j)
      map_[best_place.x + i][best_place.y + j].move_away = true;
  }
}

void BuildingPlanner::run() {
  for (const auto* enemy : state_->enemies) {
    auto attack = state_->props.at(enemy->entityType).attack;
    if (!attack) continue;
    const int range = attack->attackRange + 1;
    Vec2Int smallest(std::max(0, enemy->position.x - range),
                     std::max(0, enemy->position.y - range));
    Vec2Int largest(std::min((int)map_.size(), enemy->position.x + range + 1),
                    std::min((int)map_.size(), enemy->position.y + range + 1));
    for (int i = smallest.x; i < largest.x; ++i) {
      for (int j = smallest.y; j < largest.y; ++j) {
        if (std::ceil(r_dist(Vec2Int(i, j), enemy->position)) <= range) {
          map_[i][j].move_away = true;
        }
      }
    }
  }

  for (const auto* drone : state_->drones) {
    if (map_[drone->position.x][drone->position.y].move_away) {
      Vec2Int new_place = nearestFreePlace(drone->position);
      map_[new_place.x][new_place.y].taken_by = true;
      commands_[drone->id] = Command(new_place, new_place, BUILDER_UNIT);
    }
  }
}

void BuildingPlanner::dig() {
  typedef std::pair<int, std::pair<int, Command>> queue_contents;
  std::priority_queue<queue_contents, std::vector<queue_contents>,
                      std::greater<queue_contents>>
      orders;

  const auto digging_places = diggingPlaces();
  for (const auto* drone : state_->drones) {
    if (commands_.count(drone->id)) continue;
    for (const auto& place : digging_places) {
      if (map_[place.second.x][place.second.y].move_away ||
          map_[place.second.x][place.second.y].taken_by) {
        continue;
      }
      orders.push(std::make_pair(
          std::max(place.first.x, place.first.y) / 2 +
              dist(drone->position, place.second),
          std::make_pair(drone->id,
                         Command(place.first, place.second, RESOURCE))));
    }
  }

  while (!orders.empty()) {
    const auto& command = orders.top().second;
    if (!commands_.count(command.first) &&
        !map_[command.second.move_to.x][command.second.move_to.y].taken_by) {
      commands_[command.first] = command.second;
      map_[command.second.move_to.x][command.second.move_to.y].taken_by = true;
    }
    orders.pop();
  }
}

Vec2Int BuildingPlanner::nearestFreePlace(Vec2Int pos) const {
  bool found     = false;
  Vec2Int result = pos;
  for (int i = 0; i < state_->map.size(); ++i) {
    for (int j = 0; j < state_->map.size(); ++j) {
      const Vec2Int new_pos(i, j);
      if (!isFree(*state_, new_pos, false) || map_[i][j].taken_by ||
          map_[i][j].move_away)
        continue;
      if (!found || dist(new_pos, pos) < dist(result, pos)) {
        result = new_pos;
        found  = true;
      }
    }
  }
  return result;
}

int BuildingPlanner::nearestFreeDrone(Vec2Int position) const {
  int best_dist = 0;
  int drone_id  = -1;
  for (const auto* drone : state_->drones) {
    if (commands_.count(drone->id)) continue;
    const int d = dist(position, drone->position);
    if (drone_id != -1 && d >= best_dist) continue;
    best_dist = d;
    drone_id  = drone->id;
  }
  return drone_id;
}

Vec2Int BuildingPlanner::nearestFreePlacing(EntityType type) const {
  int best_distance = -1;
  Vec2Int best_result(-1, -1);

  const int size = state_->props.at(type).size;

  for (int i = 0; i < state_->map.size() - size; ++i) {
    for (int j = 0; j < state_->map.size() - size; ++j) {
      Vec2Int cpos(i, j);
      if (best_distance != -1 && (remoteness(cpos) >= best_distance)) continue;

      // Whole space should be free or contain only worker drones.
      bool free_space = true;
      for (int io = 0; io < size; ++io) {
        for (int jo = 0; jo < size; ++jo) {
          if (!isFree(*state_, Vec2Int(i + io, j + jo))) {
            free_space = false;
            break;
          }
        }
      }
      if (!free_space) continue;

      // Frame around could have one continuos line of filled cells.
      // Otherwise, we will be building between two openings and may mess up
      // some passage.
      Vec2Int apos(cpos.x - 1, cpos.y - 1);
      bool prev_free                     = isFree(*state_, apos);
      int changes                        = 0;
      const std::vector<Vec2Int> offsets = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
      for (int pass = 0; pass < 4; ++pass) {
        for (int pos = 0; pos < size + 1; ++pos) {
          if (isFree(*state_, apos) != prev_free) ++changes;
          prev_free = isFree(*state_, apos);
          apos.x += offsets[pass].x;
          apos.y += offsets[pass].y;
        }
      }
      if (changes > 2) continue;

      best_distance = remoteness(cpos);
      best_result   = cpos;
    }
  }

  return best_result;
}

std::vector<std::pair<Vec2Int, Vec2Int>> BuildingPlanner::diggingPlaces()
    const {
  std::vector<std::pair<Vec2Int, Vec2Int>> results;
  for (const auto* resource : state_->resources) {
    if (isFree(*state_, Vec2Int(resource->position.x + 1, resource->position.y),
               true))
      results.push_back(std::make_pair(
          resource->position,
          Vec2Int(resource->position.x + 1, resource->position.y)));

    if (isFree(*state_, Vec2Int(resource->position.x - 1, resource->position.y),
               true))
      results.push_back(std::make_pair(
          resource->position,
          Vec2Int(resource->position.x - 1, resource->position.y)));

    if (isFree(*state_, Vec2Int(resource->position.x, resource->position.y + 1),
               true))
      results.push_back(std::make_pair(
          resource->position,
          Vec2Int(resource->position.x, resource->position.y + 1)));

    if (isFree(*state_, Vec2Int(resource->position.x, resource->position.y - 1),
               true))
      results.push_back(std::make_pair(
          resource->position,
          Vec2Int(resource->position.x, resource->position.y - 1)));
  }
  return results;
}
