#pragma once

#include <queue>
#include <unordered_set>

#include "brain/state.h"
#include "brain/utils.h"
#include "model/Model.hpp"

class BuildingPlanner {
 public:
  void update();
  EntityAction command(const Entity* entity);

 private:
  struct Command {
    Command()                     = default;
    Command(const Command& other) = default;
    Command(Vec2Int drone_position, EntityType target_type)
        : drone_position(drone_position), target_type(target_type){};
    Command(Vec2Int target_position, Vec2Int drone_position,
            EntityType target_type)
        : drone_position(drone_position),
          target_position(target_position),
          target_type(target_type){};

    bool operator<(const Command& other) const {
      return target_type < other.target_type;
    }

    Vec2Int drone_position;
    Vec2Int target_position;
    EntityType target_type;
  };

  void repair();
  void build(EntityType type);
  void run();
  void dig();
  void rampage();

  std::vector<Vec2Int> builderPlacings(Vec2Int position,
                                       EntityType building_type) const;
  std::pair<Vec2Int, int> droneForBuilding(Vec2Int position,
                                           EntityType building_type) const;
  Vec2Int nearestFreePlacing(EntityType type) const;
  Vec2Int nearestFreePlace(Vec2Int pos) const;

  std::vector<std::pair<Vec2Int, Vec2Int>> diggingPlaces() const;

  std::unordered_map<int, Command> commands_;
};
