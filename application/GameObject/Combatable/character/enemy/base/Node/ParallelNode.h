#pragma once
#include "CompositeNode.h"
#include <algorithm>

/**
 * \brief 複数の子ノードを同時にTickし、成功数に応じて判定するパラレルノード
 * successThreshold個以上成功したらSuccess、failureThreshold個以上失敗したらFailure
 */
class ParallelNode : public CompositeNode
{
public:
    ParallelNode(size_t successThreshold, size_t failureThreshold)
        : successThreshold_(successThreshold), failureThreshold_(failureThreshold)
    {
		nodeName_ = "ParallelNode";
    }

    NodeStatus Tick(Blackboard& blackboard) override;

    void Reset() override;

private:
    size_t successThreshold_;
    size_t failureThreshold_;
};