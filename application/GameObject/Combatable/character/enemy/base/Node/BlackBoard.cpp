#include "BlackBoard.h"

bool Blackboard::Has(const std::string& key) const
{
	return data_.find(key) != data_.end();
}
