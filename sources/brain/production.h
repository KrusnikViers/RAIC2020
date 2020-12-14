#pragma once

#include "brain/state.h"
#include "brain/utils.h"

class ProductionPlanner {
 public:
  void update();
  EntityAction command(const Entity* entity);

 private:
};
