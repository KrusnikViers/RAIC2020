#pragma once

#include "brain/building.h"
#include "brain/fighting.h"
#include "brain/production.h"
#include "brain/state.h"
#include "brain/utils.h"
#include "model/Model.hpp"

class Brain {
 public:
  Action update(const PlayerView& playerView);

 private:
  BuildingPlanner building_;
  FightingPlanner fighting_;
  ProductionPlanner production_;
};
