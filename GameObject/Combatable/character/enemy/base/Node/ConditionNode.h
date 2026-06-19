#pragma once
#include "BTNode.h"
#include <functional>


/**
 * \brief 条件チェックノード
 */
class ConditionNode : public BTNode
{
public:
    using ConditionFunction = std::function<bool(Blackboard&)>;

    ConditionNode(ConditionFunction func) : condition(func)
    {
		nodeName_ = "ConditionNode";
    }

    NodeStatus Tick(Blackboard& blackboard) override;

    void Reset() override
    {
        // 条件ノードも状態は持たないので特になし
    }

private:
    ConditionFunction condition;

};
