#include "brain/building_queue.h"

void BuildingQueue::update(const PlayerView& view, const State& state) {
  being_digged_.clear();
  digging_.clear();
  building_.clear();
  if (waiting_) {
    const Entity* entity = state.map()[waiting_->pos.x][waiting_->pos.y];
    if (entity && !view.entityProperties.at(entity->entityType).canMove) {
      waiting_ = nullptr;
    }
  }

  for (const auto& entity : state.entities()) {
    if (!entity.second->playerId || *entity.second->playerId != state.)
}

void BuildingQueue::enqueue(const PlayerView& view, const State& state,
                            EntityType type) {}

EntityAction BuildingQueue::commandsForBuilder(const Entity* entity) {
  return EntityAction();
}

// const Player* me = nullptr;
// for (const auto& player : view.players) {
//  if (player.id == id_) me = &player;
//}
// if (builder_id_ == entity.id) {
//  const Entity* unbuilt_house = nullptr;
//  for (const auto& house : view.entities) {
//    if (house.entityType == HOUSE && *house.playerId == id_ &&
//        house.health < 50 &&
//        (!unbuilt_house ||
//         dist(entity.position, house.position) <=
//             dist(entity.position, unbuilt_house->position))) {
//      unbuilt_house = &house;
//    }
//  }
//
//  if (unbuilt_house) {
//    result.entityActions[entity.id] = EntityAction(
//        std::make_shared<MoveAction>(unbuilt_house->position, true, true),
//        nullptr, nullptr, std::make_shared<RepairAction>(unbuilt_house->id));
//    break;
//  } else if (me && me->resource >= 100) {
//    const auto new_house = getNearestPlacing(entity.position);
//    if (new_house.x != -1) {
//      result.entityActions[entity.id] = EntityAction(
//          std::make_shared<MoveAction>(new_house, true, true),
//          std::make_shared<BuildAction>(HOUSE, new_house), nullptr, nullptr);
//      break;
//    }
//  }
//}
//
// if (assigned_.count(entity.id)) break;
// const auto* resource = getNearest(entity.position, Resource, true);
// if (resource) {
//  result.entityActions[entity.id] = EntityAction(
//      std::make_shared<MoveAction>(resource->position, true, true), nullptr,
//      std::make_shared<AttackAction>(std::make_shared<int>(resource->id),
//                                     nullptr),
//      nullptr);
//  if (assigned_.count(entity.id)) targeted_.erase(assigned_[entity.id]);
//  assigned_[entity.id]    = resource->id;
//  targeted_[resource->id] = entity.id;
//}
// break;