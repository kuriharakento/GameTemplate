#pragma once
#include "CompositeNode.h"


/**
 * \brief 子ノードを順に実行し、最初に成功したノードのステータスを返す（OR条件）
 */
class SelectorNode : public CompositeNode
{
public:
    SelectorNode()
    {
		nodeName_ = "SelectorNode";
    }
    NodeStatus Tick(Blackboard& blackboard) override;
};