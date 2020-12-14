#ifndef _MODEL_ENTITY_TYPE_HPP_
#define _MODEL_ENTITY_TYPE_HPP_

#include "strategy/Stream.hpp"

enum EntityType {
  WALL      = 0,
  SUPPLY    = 1,
  BASE      = 2,
  DRONE     = 3,
  MBARRACKS = 4,
  MELEE     = 5,
  BARRACKS  = 6,
  RANGED    = 7,
  RESOURCE  = 8,
  TURRET    = 9,
  NONE      = 10,
};

#endif
