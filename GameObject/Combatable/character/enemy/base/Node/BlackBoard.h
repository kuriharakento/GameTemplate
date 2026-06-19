#pragma once
#include <unordered_map>
#include <string>
#include <any>
#include <stdexcept>

/**
 * \brief ノード間で共有する情報を格納するクラス
 */
class Blackboard
{
public:
    template<typename T>
    void Set(const std::string& key, const T& value)
    {
		data_[key] = std::any(value);
    }

    template<typename T>
    T Get(const std::string& key) const
    {
        auto it = data_.find(key);
        if (it == data_.end()) throw std::runtime_error("Key not found in Blackboard: " + key);
        return std::any_cast<T>(it->second);
    }

    template<typename T>
    bool TryGet(const std::string& key, T& outValue) const
    {
        auto it = data_.find(key);
        if (it == data_.end()) return false;
        if (const T* value = std::any_cast<T>(&(it->second)))
        {
            outValue = *value;
            return true;
        }
        return false;
    }

    bool Has(const std::string& key) const;

private:
    std::unordered_map<std::string, std::any> data_;
};