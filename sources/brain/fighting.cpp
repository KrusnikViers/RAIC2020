#include "brain/fighting.h"

#include "brain/utils-inl.h"

namespace {

const int kApproachDistance = 20;
const int kThreatDistance   = 10;
const int kAttackDistance   = 5;

const int kEnoughForAssault = 15;

int getGuardFine(FightingPlanner::ThreatClass threat) {
  switch (threat) {
    case FightingPlanner::Attack:
      return 3;
    case FightingPlanner::Threat:
      return 2;
    case FightingPlanner::Approach:
      return 1;
    default:
      return 4;
  }
}

}  // namespace

void FightingPlanner::update(const PlayerView& view, State& state) {
  // Update moving state
  for (auto it = guard_.begin(); it != guard_.end();) {
    auto cur = it++;
    if (!state.all.count(cur->first)) {
      if (guard_posts_.count(cur->first)) guard_posts_.erase(cur->first);
      guard_.erase(cur);
    } else
      cur->second = state.all.at(cur->first);
  }
  for (auto it = assault_.begin(); it != assault_.end();) {
    auto cur = it++;
    if (!state.all.count(cur->first))
      assault_.erase(cur);
    else
      cur->second = state.all.at(cur->first);
  }

  // Regenerate per-tick stuff
  if (heatmap_.empty()) {
    heatmap_.resize(state.map.size());
    for (auto& row : heatmap_) row.resize(state.map.size());
  }
  for (auto& row : heatmap_)
    for (auto& cell : row) cell = Neutral;
  fillHeatMap(state, state.bases);
  fillHeatMap(state, state.m_barracks);
  fillHeatMap(state, state.r_barracks);
  fillHeatMap(state, state.supplies);
  fillHeatMap(state, state.turrets);

  attackers_.clear();
  for (const auto* enemy : state.enemies) {
    if (heatmap_[enemy->position.x][enemy->position.y] > Neutral)
      attackers_[enemy->id] = enemy;
  }

  // Game logic
  // Assigning new units to guard
  for (const auto& unit : state.battle_units) {
    if (!guard_.count(unit->id) && !assault_.count(unit->id))
      guard_[unit->id] = unit;
  }

  // Backing off assault to guard, if needed
  danger_ = (guard_.size() < attackers_.size() + 2);
  while (guard_.size() < attackers_.size() && !assault_.empty()) {
    auto it = assault_.begin();
    guard_.insert(*it);
    assault_.erase(it);
  }

  // If we're not under attack - initiate assault on my own.
  const int min_guard  = 20;
  const int guard_size = static_cast<int>(guard_.size());
  if (attackers_.empty() && guard_size - min_guard > kEnoughForAssault) {
    while (guard_.size() > min_guard) {
      assault_[guard_.begin()->first] = guard_.begin()->second;
      guard_.erase(guard_.begin());
    }
  }

  full_guard_ = (guard_.size() > attackers_.size() && guard_size > min_guard);
}

EntityAction FightingPlanner::command(const State& state,
                                      const Entity* entity) {
  if (assault_.count(entity->id)) {
    const Entity* enemy = getNearestEnemy(state, entity, false);
    if (!enemy) return EntityAction(nullptr, nullptr, nullptr, nullptr);
    targeted_[enemy->id] = enemy;
    return EntityAction(
        std::make_shared<MoveAction>(enemy->position, true, true), nullptr,
        std::make_shared<AttackAction>(std::make_shared<int>(enemy->id),
                                       nullptr),
        nullptr);
  } else if (!attackers_.empty()) {
    guard_posts_.clear();
    const Entity* enemy = getNearestEnemy(state, entity, true);
    if (!enemy) return EntityAction(nullptr, nullptr, nullptr, nullptr);
    targeted_[enemy->id] = enemy;
    return EntityAction(
        std::make_shared<MoveAction>(enemy->position, true, true), nullptr,
        std::make_shared<AttackAction>(std::make_shared<int>(enemy->id),
                                       nullptr),
        nullptr);
  } else {
    Vec2Int new_pos = getBestGuardPosition(state, entity);
    if (new_pos == entity->position)
      return EntityAction(nullptr, nullptr, nullptr, nullptr);
    return EntityAction(std::make_shared<MoveAction>(new_pos, true, true),
                        nullptr, nullptr, nullptr);
  }
}

const Entity* FightingPlanner::getNearestEnemy(const State& state,
                                               const Entity* unit, bool guard) {
  const Entity* result = nullptr;
  int best_score       = 0;
  bool found           = false;
  for (const auto* enemy : state.enemies) {
    if (guard && !attackers_.empty() && !attackers_.count(enemy->id)) continue;
    int score = dist(unit->position, enemy->position) -
                (state.props.at(enemy->entityType).attack ? 5 : 0) -
                (targeted_.count(enemy->id) ? 10 : 0);
    if (!found || score < best_score) {
      found      = true;
      best_score = score;
      result     = enemy;
    }
  }
  return result;
}

Vec2Int FightingPlanner::getBestGuardPosition(const State& state,
                                              const Entity* unit) {
  if (guard_posts_.count(unit->id)) {
    const Entity* content =
        state.map[guard_posts_[unit->id].x][guard_posts_[unit->id].y];
    if (!content || state.props.at(content->entityType).canMove)
      return guard_posts_[unit->id];
  }

  Vec2Int result  = unit->position;
  int best_area   = getGuardFine(heatmap_[result.x][result.y]);
  int best_result = std::lround(remoteness(result));
  if (result.x & 1 || result.y & 1) best_area = 5;

  for (int i = 0; i < heatmap_.size(); i += 2) {
    for (int j = 0; j < heatmap_.size(); j += 2) {
      if (state.map[i][j] != nullptr) continue;
      bool free_post = true;
      for (const auto& post : guard_posts_) {
        if (post.second.x == i && post.second.y == j) {
          free_post = false;
          break;
        }
      }
      if (!free_post) continue;
      int area  = getGuardFine(heatmap_[i][j]);
      int score = std::lround(remoteness(Vec2Int(i, j)));
      if (area < best_area || (area == best_area && score < best_result)) {
        result                 = Vec2Int(i, j);
        guard_posts_[unit->id] = result;
        best_area              = area;
        best_result            = score;
      }
    }
  }

  return result;
}

void FightingPlanner::fillHeatMap(const State& state, State::EntityList list) {
  if (list.empty()) return;
  const int size     = state.props.at(list[0]->entityType).size;
  const int map_size = static_cast<int>(state.map.size());

  for (const auto* entity : list) {
    Vec2Int center(entity->position.x + size / 2,
                   entity->position.y + size / 2);
    const int l = std::max(0, entity->position.x - kApproachDistance);
    const int r =
        std::min(map_size - 1, entity->position.x + size + kApproachDistance);
    const int b = std::max(0, entity->position.y - kApproachDistance);
    const int t =
        std::min(map_size - 1, entity->position.y + size + kApproachDistance);
    for (int i = l; i <= r; ++i) {
      for (int j = l; j <= r; ++j) {
        ThreatClass threat = Neutral;
        int distance = std::lround(r_dist(Vec2Int(i, j), center) - size / 2);
        if (distance <= kAttackDistance)
          threat = Attack;
        else if (distance <= kThreatDistance)
          threat = Threat;
        else if (distance <= kApproachDistance)
          threat = Approach;

        if (heatmap_[i][j] < threat) heatmap_[i][j] = threat;
      }
    }
  }
}
