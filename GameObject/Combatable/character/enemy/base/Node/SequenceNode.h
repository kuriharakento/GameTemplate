#pragma once
#include "CompositeNode.h"

/**
 * \brief 子ノードを順に実行し、最初に成功したノードのステータスを返す（AND条件）
 */
class SequenceNode : public CompositeNode
{
public:
	SequenceNode()
	{
		nodeName_ = "SequenceNode";
	}
    NodeStatus Tick(Blackboard& blackboard) override;
};