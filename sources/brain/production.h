#pragma once

#include "brain/state.h"
#include "brain/utils.h"

class ProductionPlanner {
 public:
  void update();
  EntityAction command(const Entity* entity);

 private:
  int resourceLeft() const { return state().resource - resource_dispatched_; }

  int resource_dispatched_ = 0;
};
