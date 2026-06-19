#pragma once
#include "BTNode.h"
#include <memory>

/**
 * \brief 子ノードの成否を反転するデコレータノード
 * Success→Failure、Failure→Success、Runningはそのまま
 */
class InverterNode : public BTNode
{
public:
    explicit InverterNode(std::unique_ptr<BTNode> child)
        : child_(std::move(child))
    {
		nodeName_ = "InverterNode";
    }

    NodeStatus Tick(Blackboard& blackboard) override;

    void Reset() override;

private:
    std::unique_ptr<BTNode> child_;
};