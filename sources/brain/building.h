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
    Command(Vec2Int p, EntityType t) : pos(p), type(t){};
    Command(Vec2Int p, Vec2Int p2, EntityType t)
        : pos(p), move_to(p2), type(t){};

    bool operator<(const Command& other) const { return type < other.type; }

    Vec2Int pos;
    Vec2Int move_to;
    EntityType type;
  };

  void repair(const State::EntityList& list);
  void build(EntityType type);
  void run();
  void dig();

  int nearestFreeDrone(Vec2Int position) const;
  Vec2Int nearestFreePlacing(EntityType type) const;
  Vec2Int nearestFreePlace(Vec2Int pos) const;

  std::vector<std::pair<Vec2Int, Vec2Int>> diggingPlaces() const;

  std::unordered_map<int, Command> commands_;
};
