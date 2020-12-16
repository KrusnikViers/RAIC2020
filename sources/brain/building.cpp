#include "brain/building.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>

void BuildingPlanner::update() {
  commands_.clear();
  repair();
  for (EntityType type : {BASE, BARRACKS, SUPPLY})
    if (state().production_queue.count(type)) build(type);
  run();
  dig();
}

EntityAction BuildingPlanner::command(const Entity* entity) {
  if (commands_.count(entity->id)) {
    const Command& command = commands_[entity->id];
    const Entity* target   = state().cell(command.target_position).entity;
    if (command.target_type == RESOURCE) {
      return EntityAction(actionMove(command.drone_position, false), nullptr,
                          actionAttack(target->id), nullptr);
    } else if (command.target_type == DRONE) {
      return EntityAction(actionMove(command.drone_position, false), nullptr,
                          nullptr, nullptr);
    } else if (!target || target->entityType != command.target_type) {
      return EntityAction(actionMove(command.drone_position, false),
                          std::make_shared<BuildAction>(
                              command.target_type, command.target_position),
                          nullptr, nullptr);
    } else {
      return EntityAction(actionMove(command.drone_position, false), nullptr,
                          nullptr, std::make_shared<RepairAction>(target->id));
    }
  }

  return EntityAction(nullptr, nullptr, nullptr, nullptr);
}

void BuildingPlanner::repair() {
  repair(BASE);
  repair(BARRACKS);
  repair(SUPPLY);
}

void BuildingPlanner::repair(EntityType type) {
  const int size = state().props[type].size;
  for (const auto* building : state().my(type)) {
    if (building->health == state().props[type].maxHealth) continue;
    int repair_drones = (size + 1) / 2;
    for (int i = 0; i < repair_drones; ++i) {
      const auto repair_placing = droneForBuilding(building->position, type);
      if (repair_placing.second == -1) break;
      commands_[repair_placing.second] =
          Command(building->position, repair_placing.first, type);
      state().cell(repair_placing.first).purpose = DronePosition;
    }
  }
}

void BuildingPlanner::build(EntityType type) {
  if (state().resource < state().props[type].initialCost) return;
  Vec2Int best_place = nearestFreePlacing(type);
  if (best_place.x == -1) return;

  const int size           = state().props.at(type).size;
  const auto build_placing = droneForBuilding(best_place, type);
  if (build_placing.second == -1) return;

  commands_[build_placing.second] =
      Command(best_place, build_placing.first, type);
  state().cell(build_placing.first).purpose = DronePosition;
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      state().cell(best_place.x + i, best_place.y + j).attack_status = Threat;
    }
  }
}

void BuildingPlanner::run() {
  for (const auto* drone : state().my(DRONE)) {
    if (commands_.count(drone->id)) continue;
    if (state().cell(drone->position).attack_status != Safe) {
      Vec2Int new_place               = nearestFreePlace(drone->position);
      state().cell(new_place).purpose = DronePosition;
      commands_[drone->id]            = Command(new_place, new_place, DRONE);
    }
  }
}

void BuildingPlanner::dig() {
  // Position score, drone id, command.
  typedef std::pair<int, std::pair<int, Command>> queue_contents;
  std::priority_queue<queue_contents, std::vector<queue_contents>,
                      std::greater<queue_contents>>
      orders;

  const auto digging_places = diggingPlaces();
  for (const auto* drone : state().my(DRONE)) {
    if (commands_.count(drone->id)) continue;
    for (const auto& place : digging_places) {
      orders.push(std::make_pair(
          std::max(place.first.x, place.first.y) / 4 +
              m_dist(drone->position, place.second),
          std::make_pair(drone->id,
                         Command(place.first, place.second, RESOURCE))));
    }
  }

  while (!orders.empty()) {
    const auto& command = orders.top().second;
    if (!commands_.count(command.first) &&
        !state().cell(command.second.drone_position).purpose != NoPurpose) {
      commands_[command.first]                            = command.second;
      state().cell(command.second.drone_position).purpose = DronePosition;
    }
    orders.pop();
  }
}

Vec2Int BuildingPlanner::nearestFreePlace(Vec2Int pos) const {
  bool found     = false;
  Vec2Int result = pos;
  for (int i = 0; i < state().map_size; ++i) {
    for (int j = 0; j < state().map_size; ++j) {
      const Vec2Int new_pos(i, j);
      if (!isFree(new_pos.x, new_pos.y) ||
          state().cell(i, j).purpose != NoPurpose ||
          state().cell(i, j).attack_status != Safe) {
        continue;
      }
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
    if (isFree(cell.x, cell.y, AllowDrone) &&
        state().cell(cell).purpose == NoPurpose &&
        state().cell(cell).attack_status == Safe) {
      cells_available.push_back(cell);
    }
  }
  return cells_available;
}

std::pair<Vec2Int, int> BuildingPlanner::droneForBuilding(
    Vec2Int position, EntityType building_type) const {
  int best_dist = -1;
  std::pair<Vec2Int, int> result(Vec2Int(), -1);

  const auto builders_placings = builderPlacings(position, building_type);
  for (const auto& placing : builders_placings) {
    for (const auto* drone : state().my(DRONE)) {
      if (commands_.count(drone->id)) continue;
      const int d = m_dist(placing, drone->position);
      if (result.second == -1 || d < best_dist) {
        result.first  = placing;
        result.second = drone->id;
        best_dist     = d;
      }
    }
  }
  return result;
}

Vec2Int BuildingPlanner::nearestFreePlacing(EntityType type) const {
  int best_score = -1;
  Vec2Int best_result(-1, -1);
  const int size = state().props.at(type).size;

  for (int i = 0; i < state().map_size - size; ++i) {
    for (int j = 0; j < state().map_size - size; ++j) {
      Vec2Int cpos(i, j);
      // Whole space should be safe and free or contain only worker drones.
      bool safe_space = true;
      for (int io = 0; io < size; ++io) {
        for (int jo = 0; jo < size; ++jo) {
          if (!isFree(i + io, j + jo, AllowDrone) ||
              state().cell(i + io, j + jo).attack_status != Safe ||
              state().cell(i + io, j + jo).purpose != NoPurpose) {
            safe_space = false;
            break;
          }
        }
      }
      if (!safe_space) continue;

      // Frame around could have one continuos line of filled cells.
      // Otherwise, we will be building between two openings and may mess up
      // some passage.
      const auto frame_cells = frameCells(cpos.x, cpos.y, size);
      bool prev_free =
          isFree(frame_cells.back().x, frame_cells.back().y, AllowUnit);
      int changes                = 0;
      bool has_place_for_builder = false;
      for (const auto& point : frame_cells) {
        if (!isFree(point.x, point.y, AllowUnit) && !isOut(point.x, point.y)) {
          safe_space = false;
        }
        if (!isOut(point.x, point.y) &&
            state().cell(point).purpose == NoPurpose) {
          has_place_for_builder = true;
        }
        const bool now_free = isFree(point.x, point.y, AllowUnit);
        if (now_free != prev_free) {
          ++changes;
          prev_free = now_free;
        }
      }
      if (size < 4) {
        if (changes > 2) continue;
        if (changes == 0 && !prev_free) continue;
      } else {
        if (!safe_space) continue;
      }
      if (!has_place_for_builder) continue;

      int drone_score = -1;
      for (const auto& drone : state().my(DRONE)) {
        if (commands_.count(drone->id)) continue;
        const int drone_dist =
            std::max(size + 1, m_dist(drone->position, cpos));
        if (drone_score == -1 || drone_dist < drone_score)
          drone_score = drone_dist;
      }

      // We don't actually have any free drones, in this case.
      if (drone_score == -1) return best_result;
      int score = m_dist(cpos, Vec2Int(0, 0)) / 4 + drone_score;
      if (best_score == -1 || score < best_score) {
        best_score  = score;
        best_result = cpos;
      }
    }
  }

  return best_result;
}

std::vector<std::pair<Vec2Int, Vec2Int>> BuildingPlanner::diggingPlaces()
    const {
  std::vector<std::pair<Vec2Int, Vec2Int>> results;
  for (const auto* resource : state().resources) {
    for (const auto& point : frameCells(resource, false)) {
      if (isFree(point.x, point.y, AllowDrone) &&
          state().cell(point).attack_status == Safe &&
          state().cell(point).purpose == NoPurpose) {
        results.push_back(std::make_pair(resource->position, point));
      }
    }
  }
  return results;
}
