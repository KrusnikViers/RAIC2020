#include "brain/brain.h"

Action Brain::update(const PlayerView& view) {
  state().update(view);
  production_.update();
  building_.update();
  fighting_.update();

  Action result = Action(std::unordered_map<int, EntityAction>());

  for (const auto& entity : view.entities) {
    if (!state().mine(&entity)) continue;

    switch (entity.entityType) {
      case BASE:
      case BARRACKS:
      case MBARRACKS:
        result.entityActions[entity.id] = production_.command(&entity);
        break;
      case DRONE:
        result.entityActions[entity.id] = building_.command(&entity);
        break;
      case MELEE:
      case RANGED:
        result.entityActions[entity.id] = fighting_.command(&entity);
        break;
    }
  }

  return result;
}
