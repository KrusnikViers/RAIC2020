#include "brain/building.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>

namespace {

bool isFree(const State& state, const Vec2Int& pos, bool or_drone = true) {
  if (pos.x < 0 || pos.y < 0 || pos.x >= state.map.size() ||
      pos.y >= state.map.size())
    return false;
  const Entity* entity = state.map[pos.x][pos.y];
  return entity == nullptr || (or_drone && entity->entityType == BUILDER_UNIT &&
                               *entity->playerId == state.id);
}

}  // namespace

void BuildingPlanner::update(const PlayerView& view, State& state) {
  commands_.clear();
  move_away_.clear();

  repairBuildings(state, state.supplies);
  repairBuildings(state, state.m_barracks);
  repairBuildings(state, state.r_barracks);
  repairBuildings(state, state.bases);

  if (state.supply_used >= (state.supply_now + state.supply_building) * 0.8 &&
      state.resource >= (state.props.at(HOUSE).initialCost + 120)) {
    build(state, HOUSE);
  }

  dig(state);
}

EntityAction BuildingPlanner::command(const State& state,
                                      const Entity* entity) {
  if (move_away_.count(entity->id)) {
    int best_result = -1;
    Vec2Int best_move;
    std::vector<Vec2Int> offsets = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    std::random_shuffle(offsets.begin(), offsets.end());
    for (const auto& offset : offsets) {
      Vec2Int new_pos(entity->position.x + offset.x,
                      entity->position.y + offset.y);
      if (!isFree(state, new_pos, false)) continue;
      if (best_result != -1 &&
          (dist(new_pos, move_away_[entity->id]) / 2) <= best_result)
        continue;
      best_result = dist(new_pos, move_away_[entity->id]) / 2;
      best_move   = new_pos;
    }
    if (best_result != -1) {
      return EntityAction(std::make_shared<MoveAction>(best_move, false, false),
                          nullptr, nullptr, nullptr);
    }
  }

  if (commands_.count(entity->id)) {
    const Command& command = commands_[entity->id];
    const int size         = state.props.at(command.type).size;
    Vec2Int center(command.pos.x + size / 2, command.pos.y + size / 2);
    const Entity* target = state.map[command.pos.x][command.pos.y];

    if (command.type == RESOURCE) {
      return EntityAction(std::make_shared<MoveAction>(command.pos, true, true),
                          nullptr,
                          std::make_shared<AttackAction>(
                              std::make_shared<int>(target->id), nullptr),
                          nullptr);
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

void BuildingPlanner::repairBuildings(const State& state,
                                      const State::EntityList& list) {
  for (const auto& building : list) {
    if (!building->active) {
      int drone_id = nearestFreeDrone(building->position, state);
      if (drone_id == -1) continue;
      commands_[drone_id] = Command(building->position, building->entityType);
    }
  }
}

void BuildingPlanner::build(const State& state, EntityType type) {
  Vec2Int best_place = nearestFreePlacing(state, type);
  if (best_place.x == -1) return;

  const int size = state.props.at(type).size;
  Vec2Int center(best_place.x + size / 2, best_place.y + size / 2);

  const int drone_id = nearestFreeDrone(center, state);
  if (drone_id == -1) return;

  commands_[drone_id] = Command(best_place, type);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      if (state.map[best_place.x + i][best_place.y + j] != nullptr) {
        move_away_[state.map[best_place.x + i][best_place.y + j]->id] = center;
      }
    }
  }
}

void BuildingPlanner::dig(const State& state) {
  std::unordered_set<int> taken;
  for (const auto* drone : state.drones) {
    if (commands_.count(drone->id)) continue;
    Vec2Int resource = nearestFreeResource(state, taken, drone->position);
    if (resource.x == -1) continue;
    // taken.insert(state.map[resource.x][resource.y]->id);
    commands_[drone->id] = Command(resource, RESOURCE);
  }
}

int BuildingPlanner::nearestFreeDrone(Vec2Int position,
                                      const State& state) const {
  int best_dist = 0;
  int drone_id  = -1;
  for (const auto* drone : state.drones) {
    if (commands_.count(drone->id)) continue;
    const int d = dist(position, drone->position);
    if (drone_id != -1 && d >= best_dist) continue;
    best_dist = d;
    drone_id  = drone->id;
  }
  return drone_id;
}

Vec2Int BuildingPlanner::nearestFreePlacing(const State& state,
                                            EntityType type) const {
  int best_distance = -1;
  Vec2Int best_result(-1, -1);

  const int size = state.props.at(type).size;

  for (int i = 0; i < state.map.size() - size; ++i) {
    for (int j = 0; j < state.map.size() - size; ++j) {
      Vec2Int cpos(i, j);
      if (best_distance != -1 && (remoteness(cpos) >= best_distance)) continue;

      // Whole space should be free or contain only worker drones.
      bool free_space = true;
      for (int io = 0; io < size; ++io) {
        for (int jo = 0; jo < size; ++jo) {
          if (!isFree(state, Vec2Int(i + io, j + jo))) {
            free_space = false;
            break;
          }
        }
      }
      if (!free_space) continue;

      // Frame around could have one continuos line of filled cells. Otherwise,
      // we will be building between two openings and may mess up some passage.
      Vec2Int apos(cpos.x - 1, cpos.y - 1);
      bool prev_free                     = isFree(state, apos);
      int changes                        = 0;
      const std::vector<Vec2Int> offsets = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
      for (int pass = 0; pass < 4; ++pass) {
        for (int pos = 0; pos < size + 1; ++pos) {
          if (isFree(state, apos) != prev_free) ++changes;
          prev_free = isFree(state, apos);
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

Vec2Int BuildingPlanner::nearestFreeResource(
    const State& state, const std::unordered_set<int>& taken_resources,
    Vec2Int pos) const {
  int best_distance = -1;
  Vec2Int best_result(-1, -1);
  for (const auto* resource : state.resources) {
    if (taken_resources.count(resource->id)) continue;
    int cur_dist = dist(resource->position, pos) +
                   std::max(resource->position.x, resource->position.y) / 2;

    for (const auto* entity : state.enemies) {
      if (entity->entityType == MELEE_UNIT ||
          entity->entityType == RANGED_UNIT) {
        if (dist(entity->position, pos) < 7) {
          cur_dist += static_cast<int>(state.map.size());
          break;
        }
      }
    }

    if (best_distance != -1 && cur_dist >= best_distance) continue;
    if (dist(resource->position, pos) == 1) return resource->position;
    if (!isFree(state, Vec2Int(resource->position.x + 1, resource->position.y),
                false) &&
        !isFree(state, Vec2Int(resource->position.x - 1, resource->position.y),
                false) &&
        !isFree(state, Vec2Int(resource->position.x, resource->position.y + 1),
                false) &&
        !isFree(state, Vec2Int(resource->position.x, resource->position.y - 1),
                false))
      continue;
    best_result   = resource->position;
    best_distance = cur_dist;
  }
  return best_result;
}
