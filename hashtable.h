#ifndef HASHTABLE_H
#define HASHTABLE_H

// Project Identifier: 2C4A3C53CD5AD45A7BEA3AE5356A3B1622C9D04B

// INSTRUCTIONS:
// fill out the methods in the class below.

// You may assume that the key and value types can be copied and have default
// constructors.

// You can also assume that the key type has (==) and (!=) methods.

// You may assume that Hasher is a functor type with a
// size_t operator()(const Key&) overload.

// The key/value types aren't guaranteed to support any other operations.

// Do not add, remove, or change any of the class's member variables.
// The num_deleted counter is *optional*. You might find it helpful, but it
// is not required to pass the lab assignment.

// Do not change the Bucket type.

// SUBMISSION INSTRUCTIONS:
// Submit this file, by itself, in a .tar.gz.
// Other files will be ignored.

#include <cstdint>
#include <functional> // where std::hash lives
#include <vector>
#include <cassert> // useful for debugging!
#include <utility>
#include <iterator>

// A bucket's status tells you whether it's empty, occupied, 
// or contains a deleted element.
enum class Status : uint8_t {
    Empty,
    Occupied,
    Deleted
};

template<typename K, typename V, typename Hasher = std::hash<K>>
class HashTable {
    // used by the autograder; do not change/remove.
    friend class Verifier;
public:
    // A bucket has a status, a key, and a value.
    struct Bucket {
        // Do not modify Bucket.
        Status status = Status::Empty;
        K key;
        V val;
    };

    HashTable() {
        buckets.resize(20);
    }

    size_t size() const {
        return num_elements;
    }

    // returns a reference to the value in the bucket with the key, if it
    // already exists. Otherwise, insert it with a default value, and return
    // a reference to the resulting bucket.
    V& operator[](const K& key) {

        Hasher hasher;
        size_t desired_bucket = hasher(key) % buckets.size();

        switch (buckets[desired_bucket].status) {

        case Status::Empty:
            buckets[desired_bucket] = { Status::Occupied, key, V() };
            ++num_elements;
            break;

        case Status::Deleted: {
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first)
                desired_bucket = result.second;
            else {
                size_t where_to_insert = find_first_insertable_index(desired_bucket);
                buckets[where_to_insert] = { Status::Occupied, key, V() };
                ++num_elements;
            }
            break;
        }

        case Status::Occupied: {
            if (buckets[desired_bucket].key == key)
                return buckets[desired_bucket].val;
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first)
                desired_bucket = result.second;
            else {
                size_t where_to_insert = find_first_insertable_index(desired_bucket);
                buckets[where_to_insert] = { Status::Occupied, key, V() };
                desired_bucket = where_to_insert;
                ++num_elements;
            }
            break;
        }
        }
        int new_index = rehash_and_grow(true, key);
        if (new_index == -1)
            return buckets[desired_bucket].val;
        else
            return buckets[new_index].val;
    }

    // insert returns whether inserted successfully
    // (if the key already exists in the table, do nothing and return false).
    bool insert(const K& key, const V& val) {

        Hasher hasher;
        size_t desired_bucket = hasher(key) % buckets.size();

        switch (buckets[desired_bucket].status) {

        case Status::Empty:
            buckets[desired_bucket] = { Status::Occupied, key, val };
            ++num_elements;
            rehash_and_grow();
            return true;

        case Status::Deleted: { // do we go next or insert
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first)
                return false;
            else {
                size_t where_to_insert = find_first_insertable_index(desired_bucket);
                buckets[where_to_insert] = { Status::Occupied, key, val };
                ++num_elements;
                rehash_and_grow();
                return true;
            }
        }

        case Status::Occupied: {
            if (buckets[desired_bucket].key == key)
                return false;
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first)
                return false;
            else {
                size_t where_to_insert = find_first_insertable_index(desired_bucket);
                buckets[where_to_insert] = { Status::Occupied, key, val };
                ++num_elements;
                rehash_and_grow();
                return true;

            }
        }
        }
    }
    // for [], we want to first check if a key exists
    // then, if it doesnt, we need to go through again to see if there is an empty or deleted

    // erase returns the number of items remove (0 or 1)
    size_t erase(const K& key) {

        Hasher hasher;
        size_t desired_bucket = hasher(key) % buckets.size();

        switch (buckets[desired_bucket].status) {

        case Status::Empty:
            return 0;

        case Status::Deleted: {
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first) {
                buckets[result.second].status = Status::Deleted;
                ++num_deleted;
                --num_elements;
                return 1;
            }
            else
                return 0;
        }

        case Status::Occupied: {
            if (buckets[desired_bucket].key == key) {
                buckets[desired_bucket].status = Status::Deleted;
                ++num_deleted;
                --num_elements;
                return 1;
            }
            std::pair<bool, int> result = does_key_exist(key, desired_bucket);
            if (result.first) {
                buckets[result.second].status = Status::Deleted;
                ++num_deleted;
                --num_elements;
                return 1;
            }
            else
                return 0;
        }
        }
    }

private:
    size_t num_elements = 0;
    size_t num_deleted = 0; // OPTIONAL: you don't need to use num_deleted to pass
public:
    std::vector<Bucket> buckets;

    int rehash_and_grow(bool tracking = false, const K& track_key = K()) {

        int new_index = -1;

        double cur_load_factor = ((double)num_elements) / buckets.size();
        if (cur_load_factor <= 0.5)
            return new_index;

        Hasher hasher;

        std::vector<Bucket> new_vec(buckets.size() * 2);

        for (int i = 0; i < buckets.size(); ++i) {

            if (buckets[i].status != Status::Occupied)
                continue;

            size_t desired_bucket = hasher(buckets[i].key) % new_vec.size();

            bool placed = false;
            while (!placed) {
                switch (new_vec[desired_bucket].status) {
                case Status::Empty:
                    new_vec[desired_bucket] = buckets[i];
                    if (tracking && track_key == buckets[i].key)
                        new_index = static_cast<int>(desired_bucket);
                    placed = true;
                    break;
                case Status::Deleted:
                    desired_bucket = (desired_bucket + 1) % new_vec.size();
                    break;
                case Status::Occupied:
                    desired_bucket = (desired_bucket + 1) % new_vec.size();
                    break;
                }
            }
        }
        buckets = new_vec;


        return new_index;
    }

    // for insert, we want to insert at first delete/empty

    std::pair<bool, int> does_key_exist(const K& key, size_t idx) {

        size_t desired_bucket = (idx + 1) % buckets.size();

        while (desired_bucket != idx) {

            switch (buckets[desired_bucket].status) {

            case Status::Empty:
                return { false, -1 };

            case Status::Deleted:
                desired_bucket = (desired_bucket + 1) % buckets.size();
                break;

            case Status::Occupied:
                if (buckets[desired_bucket].key == key)
                    return { true, desired_bucket };
                desired_bucket = (desired_bucket + 1) % buckets.size();
                break;
            }
        }
        return { false, -1 };
    }

    size_t find_first_insertable_index(size_t idx) {

        size_t desired_bucket = idx;

        do {
            Status s = buckets[desired_bucket].status;
            if (s == Status::Empty || s == Status::Deleted)
                return desired_bucket;
            desired_bucket = (desired_bucket + 1) % buckets.size();
        } while (desired_bucket != idx);

        return size_t();
    }
};

#endif // HASHTABLE_H
