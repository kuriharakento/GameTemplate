#pragma once
#include "BTNode.h"
#include <functional>

/**
 * \brief 敵の移動、攻撃、その他の具体的なアクションを実行するノード
 */
class ActionNode : public BTNode
{
public:
    using ActionFunction = std::function<NodeStatus(Blackboard&)>;

	ActionNode(const std::string& name, ActionFunction func) : action(func)
	{
		nodeName_ = name;
	}

    NodeStatus Tick(Blackboard& blackboard) override;

    void Reset() override
    {
	    /* アクションには状態を保持しないので特になし */
    }

private:
    ActionFunction action;

};
