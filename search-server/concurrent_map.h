#pragma once

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <cstdlib>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {};
        

    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<std::uint64_t>(key) % buckets_.size()];
        return {std::lock_guard{bucket.mutex}, bucket.map[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& bucket: buckets_) {
            std::lock_guard guard(bucket.mutex);
            result.insert(bucket.map.begin(), bucket.map.end());
        }
        return result;
    }
    
    void Erase(const Key& key) {
        auto& bucket = buckets_[static_cast<std::uint64_t>(key) % buckets_.size()];
        std::lock_guard guard(bucket.mutex);
        bucket.map.erase(key);
    }

private:
    struct Bucket {
        std::map<Key, Value> map;
        std::mutex mutex;
    };

    std::vector<Bucket> buckets_;
};
