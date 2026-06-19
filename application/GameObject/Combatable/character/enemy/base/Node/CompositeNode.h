#pragma once
#include <memory>
#include <vector>

#include "BTNode.h"

/**
 * \brief 救数の子ノードを持ち、子ノードを順に実行するベースクラス
 */
class CompositeNode : public BTNode
{
protected:
    std::vector<std::unique_ptr<BTNode>> children;
    size_t currentIndex = 0;

public:
	CompositeNode()
	{
		nodeName_ = "CompositeNode";
	}
    void AddChild(std::unique_ptr<BTNode> child);

    void Reset() override;

	const std::vector<std::unique_ptr<BTNode>>* GetChildren() const override
	{
		return &children;
	}
};


