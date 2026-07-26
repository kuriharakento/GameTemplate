#pragma once
#include "engine/gameobject/component/collision/CollisionLayer.h"

namespace CollisionLayer
{
using KCE::GameObjectComponent::ColliderLayer;

static constexpr ColliderLayer None = 0;
static constexpr ColliderLayer Default = 1 << 0; // デフォルト
static constexpr ColliderLayer Stage = 1 << 1;   // 地形
static constexpr ColliderLayer All = 0xFFFFFFFF;
} // namespace CollisionLayer
