#include "brain/building.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>

void BuildingPlanner::update() {
  commands_.clear();

  repair(state().buildings);

  if (state().supply_used >=
          (state().supply_now + state().supply_building) * 0.75 &&
      state().resource >= 100) {
    build(HOUSE);
  }

  run();
  dig();
}

EntityAction BuildingPlanner::command(const Entity* entity) {
  if (commands_.count(entity->id)) {
    const Command& command = commands_[entity->id];
    const int size         = state().props.at(command.type).size;
    Vec2Int center(command.pos.x + size / 2, command.pos.y + size / 2);
    const Entity* target = state().map[command.pos.x][command.pos.y].entity;

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
    if (building->health < state().props.at(building->entityType).maxHealth) {
      int drone_id = nearestFreeDrone(building->position);
      if (drone_id == -1) continue;
      commands_[drone_id] = Command(building->position, building->entityType);
      state()
          .map[state().all.at(drone_id)->position.x]
              [state().all.at(drone_id)->position.y]
          .drone_planned_position = true;
    }
  }
}

void BuildingPlanner::build(EntityType type) {
  Vec2Int best_place = nearestFreePlacing(type);
  if (best_place.x == -1) return;

  const int size = state().props.at(type).size;
  Vec2Int center(best_place.x + size / 2, best_place.y + size / 2);

  const int drone_id = nearestFreeDrone(center);
  if (drone_id == -1) return;

  commands_[drone_id] = Command(best_place, type);
  state()
      .map[state().all.at(drone_id)->position.x]
          [state().all.at(drone_id)->position.y]
      .drone_planned_position = true;
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      state().map[best_place.x + i][best_place.y + j].drone_danger_area = true;
    }
  }
}

void BuildingPlanner::run() {
  for (const auto* enemy : state().enemies) {
    auto attack = state().props.at(enemy->entityType).attack;
    if (!attack) continue;
    const int range = attack->attackRange + 1;
    Vec2Int smallest(std::max(0, enemy->position.x - range),
                     std::max(0, enemy->position.y - range));
    Vec2Int largest(std::min(state().map_size, enemy->position.x + range + 1),
                    std::min(state().map_size, enemy->position.y + range + 1));
    for (int i = smallest.x; i < largest.x; ++i) {
      for (int j = smallest.y; j < largest.y; ++j) {
        if (std::ceil(r_dist(Vec2Int(i, j), enemy->position)) <= range) {
          state().map[i][j].drone_danger_area = true;
        }
      }
    }
  }

  for (const auto* drone : state().drones) {
    if (state().map[drone->position.x][drone->position.y].drone_danger_area) {
      Vec2Int new_place = nearestFreePlace(drone->position);
      state().map[new_place.x][new_place.y].drone_planned_position = true;
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
  for (const auto* drone : state().drones) {
    if (commands_.count(drone->id)) continue;
    for (const auto& place : digging_places) {
      if (state().map[place.second.x][place.second.y].drone_danger_area ||
          state().map[place.second.x][place.second.y].drone_planned_position) {
        continue;
      }
      orders.push(std::make_pair(
          std::max(place.first.x, place.first.y) / 2 +
              m_dist(drone->position, place.second),
          std::make_pair(drone->id,
                         Command(place.first, place.second, RESOURCE))));
    }
  }

  while (!orders.empty()) {
    const auto& command = orders.top().second;
    if (!commands_.count(command.first) &&
        !state()
             .map[command.second.move_to.x][command.second.move_to.y]
             .drone_planned_position) {
      commands_[command.first] = command.second;
      state()
          .map[command.second.move_to.x][command.second.move_to.y]
          .drone_planned_position = true;
    }
    orders.pop();
  }
}

Vec2Int BuildingPlanner::nearestFreePlace(Vec2Int pos) const {
  bool found     = false;
  Vec2Int result = pos;
  for (int i = 0; i < state().map.size(); ++i) {
    for (int j = 0; j < state().map.size(); ++j) {
      const Vec2Int new_pos(i, j);
      if (!isFree(new_pos.x, new_pos.y) ||
          state().map[i][j].drone_planned_position ||
          state().map[i][j].drone_danger_area)
        continue;
      if (!found || m_dist(new_pos, pos) < m_dist(result, pos)) {
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
  for (const auto* drone : state().drones) {
    if (commands_.count(drone->id)) continue;
    const int d = m_dist(position, drone->position);
    if (drone_id != -1 && d >= best_dist) continue;
    best_dist = d;
    drone_id  = drone->id;
  }
  return drone_id;
}

Vec2Int BuildingPlanner::nearestFreePlacing(EntityType type) const {
  int best_distance = -1;
  Vec2Int best_result(-1, -1);

  const int size = state().props.at(type).size;

  for (int i = 0; i < state().map.size() - size; ++i) {
    for (int j = 0; j < state().map.size() - size; ++j) {
      Vec2Int cpos(i, j);
      if (best_distance != -1 && (remoteness(cpos) >= best_distance)) continue;

      // Whole space should be free or contain only worker drones.
      bool free_space = true;
      for (int io = 0; io < size; ++io) {
        for (int jo = 0; jo < size; ++jo) {
          if (!isFree(i + io, j + jo, Drone)) {
            free_space = false;
            break;
          }
        }
      }
      if (!free_space) continue;

      // Frame around could have one continuos line of filled cells.
      // Otherwise, we will be building between two openings and may mess up
      // some passage.
      const auto frame_cells = frameCells(cpos.x, cpos.y, size);
      if (frame_cells.size() < 2) continue;

      bool prev_free = isFree(frame_cells.back().x, frame_cells.back().y, Unit);
      int changes    = 0;
      for (const auto& point : frame_cells) {
        const bool now_free = isFree(point.x, point.y, Unit);
        if (now_free != prev_free) {
          ++changes;
          prev_free = now_free;
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
  for (const auto* resource : state().resources) {
    for (const auto& point : frameCells(resource, false)) {
      if (isFree(point.x, point.y, Drone)) {
        results.push_back(std::make_pair(resource->position, point));
      }
    }
  }
  return results;
}
