#pragma once

namespace ecs
{
    /**
     * @brief 敵の固有状態を保持する。
     */
    struct EnemyStateComponent
    {
        /**
         * @brief AIステート定義。
         */
        enum class State
        {
            Idle,
            Move,
            Attack,
            Dead
        };

        int hp_ = 100;
        State currentState_ = State::Idle;
        float stateTimer_ = 0.0f;
    };
}
