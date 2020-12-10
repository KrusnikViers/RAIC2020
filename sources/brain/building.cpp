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
    const int size         = state().props.at(command.target_type).size;
    Vec2Int center(command.target_position.x + size / 2,
                   command.target_position.y + size / 2);
    const Entity* target =
        state()
            .map[command.target_position.x][command.target_position.y]
            .entity;

    if (command.target_type == RESOURCE) {
      return EntityAction(
          std::make_shared<MoveAction>(command.drone_position, true, true),
          nullptr,
          std::make_shared<AttackAction>(std::make_shared<int>(target->id),
                                         nullptr),
          nullptr);
    } else if (command.target_type == BUILDER_UNIT) {
      return EntityAction(
          std::make_shared<MoveAction>(command.drone_position, true, true),
          nullptr, nullptr, nullptr);
    } else if (!target || target->entityType != command.target_type) {
      return EntityAction(
          std::make_shared<MoveAction>(command.drone_position, true, true),
          std::make_shared<BuildAction>(command.target_type,
                                        command.target_position),
          nullptr, nullptr);
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
      const auto repair_placing =
          droneForBuilding(building->position, building->entityType);
      if (repair_placing.second == -1) continue;
      commands_[repair_placing.second] = Command(
          building->position, repair_placing.first, building->entityType);
      state()
          .map[repair_placing.first.x][repair_placing.first.y]
          .drone_planned_position = true;
    }
  }
}

void BuildingPlanner::build(EntityType type) {
  Vec2Int best_place = nearestFreePlacing(type);
  if (best_place.x == -1) return;

  const int size           = state().props.at(type).size;
  const auto build_placing = droneForBuilding(best_place, type);
  if (build_placing.second == -1) return;

  commands_[build_placing.second] =
      Command(best_place, build_placing.first, type);
  state()
      .map[build_placing.first.x][build_placing.first.y]
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
      if (commands_.count(drone->id)) {
        const auto next_pos = commands_[drone->id].drone_position;
        if (!state().map[next_pos.x][next_pos.y].drone_danger_area) continue;
      }
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
             .map[command.second.drone_position.x]
                 [command.second.drone_position.y]
             .drone_planned_position) {
      commands_[command.first] = command.second;
      state()
          .map[command.second.drone_position.x][command.second.drone_position.y]
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

std::vector<Vec2Int> BuildingPlanner::builderPlacings(
    Vec2Int position, EntityType building_type) const {
  const auto free_cells = frameCells(
      position.x, position.y, state().props.at(building_type).size, false);
  std::vector<Vec2Int> cells_available;
  for (const auto& cell : free_cells) {
    if (isOut(cell.x, cell.y) || !isFree(cell.x, cell.y) ||
        state().map[cell.x][cell.x].drone_planned_position ||
        state().map[cell.x][cell.x].drone_danger_area) {
      continue;
    }
    cells_available.push_back(cell);
  }
  return cells_available;
}

std::pair<Vec2Int, int> BuildingPlanner::droneForBuilding(
    Vec2Int position, EntityType building_type) const {
  int best_dist = 0;
  std::pair<Vec2Int, int> result(Vec2Int(), -1);

  const auto builders_placings = builderPlacings(position, building_type);
  for (const auto& placing : builders_placings) {
    for (const auto* drone : state().drones) {
      if (commands_.count(drone->id)) continue;
      const int d = m_dist(placing, drone->position);
      if (result.second == -1 || d < best_dist) {
        result.first  = placing;
        result.second = drone->id;
      }
    }
  }
  return result;
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
      if (changes == 0 && !prev_free) continue;

      // Check, that builders could actually stand somewhere.
      if (builderPlacings(cpos, type).empty()) continue;

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
