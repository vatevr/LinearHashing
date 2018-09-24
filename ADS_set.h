#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

template<typename Key, size_t N = 3>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = key_type &;
    using const_reference = const key_type &;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    using key_compare = std::less<key_type>;   // B+-Tree
    using key_equal = std::equal_to<key_type>; // Hashing
    using hasher = std::hash<key_type>;        // Hashing
    static const size_t SIZE_INVALID = (size_t) -1;
private:
    struct Bucket {
        Key keys[N];
        size_t nextFreeIndex{0};
        Bucket* overflowBucket{nullptr};

        Bucket() {}

        ~Bucket() {
            if (nullptr != overflowBucket) {
                delete overflowBucket;
                overflowBucket = nullptr;
            }
        }
    };

    friend class PrivateBucketIterator;
    class PrivateBucketIterator {
    private:
        Bucket** _table;
        size_t _index;
        size_t _size;
    public:
        using value_type = Bucket;
        using reference = value_type &;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;

        PrivateBucketIterator(Bucket** hashTable, size_t index, size_t size)
                : _table{hashTable}, _index{index}, _size{size}
        {}

        PrivateBucketIterator(const PrivateBucketIterator& other)
                : PrivateBucketIterator(other._table, other._index, other._size)
        {}

        reference operator*() const
        {
            if(_index == SIZE_INVALID)
                throw std::runtime_error("Table array exceeded");

            Bucket* bucket = _table[_index];

            return *bucket;
        };

        pointer operator->() const
        {
            if(_index == SIZE_INVALID)
                throw std::runtime_error("Table array exceeded");

            return _table[_index];
        };

        PrivateBucketIterator& operator++()
        {
            if(_index == SIZE_INVALID)
                throw std::runtime_error("Table array exceeded");

            if(++_index == _size)
                _index = SIZE_INVALID;

            return *this;
        };

        PrivateBucketIterator operator++(int)
        {
            PrivateBucketIterator other {*this};
            ++(*this);
            return other;
        };

        friend bool operator==(const PrivateBucketIterator& me, const PrivateBucketIterator& other)
        {
            return me._table == other._table
                   && me._index == other._index;
        };

        friend bool operator!=(const PrivateBucketIterator& me, const PrivateBucketIterator& other)
        {
            return !(me == other);
        };
    };

    Bucket** table_{nullptr};
    size_t tableSize_;
    size_t size_{0};
    size_t d_{2};
    size_t nextToSplit_{0};
    float maxLoadFactor_{0.9};

    using bucketIterator = PrivateBucketIterator;

    void split() {
        if (0 == nextToSplit_) {
            Bucket** tmp = new Bucket*[tableSize_ * 2];
            for (size_t i = 0; i < tableSize_; ++i) {
                tmp[i] = table_[i];
            }
            delete[] table_;
            table_ = tmp;
        }
        table_[tableSize_++] = new Bucket();
    }

    void reserve(size_t n) {
        // instead of capacity we tweak buckets in main directory
        if (n / float(N * tableSize_) > maxLoadFactor_) {
            split();
            rehash(nextToSplit_++);
            // Splitting is through
            if (1 << d_ == nextToSplit_) {
                ++d_;
                nextToSplit_ = 0;
            }
        }
    }


    size_t bucketAddress(const Key& key) const {
        size_t n = hasher{}(key);
        if (n % (size_t)(1 << d_) >= nextToSplit_)
            return n % (size_t)(1 << d_);
        else
            return n % (size_t)(1 << (d_ + 1));
    }

    void rehash(size_t index) {
        Bucket* bucket = table_[index];

        size_t address = index + (1 << d_);
        Bucket* splittedBucketToStore = table_[address];

        while (bucket) {
            for (size_t i = 0; i < bucket->nextFreeIndex; ++i) {
                if (bucketAddress(bucket->keys[i]) != index) {
                    if (splittedBucketToStore->nextFreeIndex == N) {
                        splittedBucketToStore->overflowBucket = new Bucket();
                        splittedBucketToStore = splittedBucketToStore->overflowBucket;
                    }

                    splittedBucketToStore->keys[splittedBucketToStore->nextFreeIndex] = bucket->keys[i];
                    ++splittedBucketToStore->nextFreeIndex;

                    // restores
                    for(size_t j = i + 1; j < bucket->nextFreeIndex; ++j) {
                        bucket->keys[j - 1] = bucket->keys[j];
                    }
                    --bucket->nextFreeIndex;
                    // Recalculate again since moved back
                    --i;
                }
            }

            bucket = bucket->overflowBucket;
        }
    }

    iterator insertUnchecked(const key_type &key) {
        size_type address = bucketAddress(key);
        Bucket* bucket = table_[address];

        while(bucket->nextFreeIndex > N - 1) {
            if (nullptr == bucket->overflowBucket) {
                bucket->overflowBucket = new Bucket();
            }
            bucket = bucket->overflowBucket;
        }

        size_t savedAtIndex = bucket->nextFreeIndex;
        bucket->keys[savedAtIndex] = key;
        ++size_;
        ++bucket->nextFreeIndex;
        return iterator{bucketBegin(address), bucketEnd(), bucket, savedAtIndex};

    }

    bucketIterator bucketBegin(size_t index) const { return bucketIterator(this->table_, index, tableSize_); }
    bucketIterator bucketEnd() const { return bucketIterator(this->table_, SIZE_INVALID, tableSize_); }

public:
    ADS_set() {
        tableSize_ = (size_t)(1<<d_);
        table_ = new Bucket*[tableSize_];
        for (size_t i = 0; i < tableSize_; ++i) {
            table_[i] = new Bucket();
        }
    }

    ADS_set(std::initializer_list<key_type> ilist): ADS_set{} {
        insert(ilist);
    };
    template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{} { insert(first, last); }

    ADS_set(const ADS_set& other): ADS_set{} {
        for (const_iterator it = other.begin(); it != other.end(); ++it){
            insert(*it);
        }
    }

    ~ADS_set() {
        for (size_t i = 0; i < tableSize_; ++i) {
            delete table_[i];
        }

        delete[] table_;
    }

    ADS_set &operator=(const ADS_set &other) {
        if (this == &other) return *this;
        ADS_set tmp{other};
        swap(tmp);
        return *this;
    }
    ADS_set &operator=(std::initializer_list<key_type> ilist) {
        ADS_set tmp{ilist};
        swap(tmp);
        return *this;
    }

    size_type size() const { return size_; }

    bool empty() const { return !size_;};

    // Wie oft gegebene Wert gespeichert ist
    size_type count(const key_type& key) const {
        if (empty()) {
            return 0;
        }


        size_type index = bucketAddress(key);

        Bucket* bucket = table_[index];

        while (bucket) {
            for (size_type i{0}; i < bucket->nextFreeIndex; ++i) {
                if (key_equal{}(key, bucket->keys[i])) {
                    return 1;
                }
            }

            bucket = bucket->overflowBucket;
        }

        return 0;
    };

    iterator find(const key_type& key) const {
        size_t index = bucketAddress(key);
        Bucket* bucket = table_[index];
        while (bucket) {
            for (size_t i = 0; i < bucket->nextFreeIndex; ++i) {
                if (key_equal{}(key, bucket->keys[i])) {
                    return iterator{bucketBegin(index), bucketEnd(), bucket, i};
                }
            }

            bucket = bucket->overflowBucket;
        }

        return end();
    };

    void clear() {
        ADS_set tmp;
        swap(tmp);
    }

    void swap(ADS_set &other) {
        std::swap(table_, other.table_);
        std::swap(d_, other.d_);
        std::swap(nextToSplit_, other.nextToSplit_);
        std::swap(size_, other.size_);
        std::swap(tableSize_, other.tableSize_);
        std::swap(maxLoadFactor_, other.maxLoadFactor_);
    }

    void insert(std::initializer_list<key_type> ilist) {
        for (const auto &key: ilist) {
            if (count(key) == 0) {
                reserve(size_ + 1);
                insertUnchecked(key);
            }
        }
    }

    std::pair<iterator, bool> insert(const key_type &key) {
        iterator it{find(key)};
        if (it != end()) {
            return {it, false};
        }

        reserve(size_ + 1);
        return {insertUnchecked(key), true};
    }

    template<typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last ; ++it) {
            if (count(*it) == 0) {
                reserve(size_ + 1);
                insertUnchecked(*it);
            }
        }
    }

    size_type erase(const key_type &key) {
        size_t index = bucketAddress(key);
        Bucket* bucket = table_[index];

        while (bucket) {
            for (size_t i = 0; i < bucket->nextFreeIndex; ++i) {
                if (key_equal{}(key, bucket->keys[i])) {
                    // Move array forward
                    --bucket->nextFreeIndex;
                    for (size_t j = i; j < bucket->nextFreeIndex; ++j) {
                        bucket->keys[j] = bucket->keys[j + 1];
                    }
                    --size_;
                    return 1;
                }
            }

            bucket = bucket->overflowBucket;
        }

        return 0;
    }

    const_iterator begin() const {
        iterator a{bucketBegin(0), bucketEnd(), table_[0], 0};

        if (table_[0]->nextFreeIndex == 0) {
            a.advanceToNext();
        }

        return a;
    }
    const_iterator end() const { return const_iterator{bucketEnd(), bucketEnd(), nullptr, SIZE_INVALID}; };

    void dump(std::ostream &o = std::cerr) const {
        for (size_t i = 0; i < tableSize_; ++i) {
            Bucket* bucket = table_[i];

            while (bucket) {
                for (size_type j{0}; j < N; ++j) {
                    j < bucket->nextFreeIndex ? o << bucket->keys[j] : o << "-";
                    o << " ";
                }

                bucket = bucket->overflowBucket;
            }

            o << "\n";
        }
    };

    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
        if (lhs.size_ != rhs.size_) return false;

        for (const_iterator it = rhs.begin(); it != rhs.end(); ++it) {
            if (lhs.find(*it) == lhs.end()) return false;
        }

        return true;
    };
    friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) {
        return !(lhs == rhs);
    };
};

template<typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
private:
    ADS_set<Key, N>::PrivateBucketIterator beginIterator_;
    ADS_set<Key, N>::PrivateBucketIterator endIterator_;
    ADS_set<Key, N>::Bucket* position_;
    size_t index_;

public:
    void advanceToNext() {
        for (size_t i = index_ + 1; i < position_->nextFreeIndex; ++i) {
            index_ = i;
            return;
        }

        position_ = position_->overflowBucket;
        while (position_) {
            index_ = 0;
            for (size_t i = index_; i < position_->nextFreeIndex; ++i) {
                index_ = i;
                return;
            }
            position_ = position_->overflowBucket;
        }

        for (++beginIterator_; beginIterator_ != endIterator_; ++beginIterator_) {
            position_ = &(*beginIterator_);
            while (position_) {
                index_ = 0;
                for (size_t i = index_; i < position_->nextFreeIndex; ++i) {
                    index_ = i;
                    return;
                }
                position_ = position_->overflowBucket;
            }
        }

        index_ = (size_t)-1;
    }
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;
    using BucketIterator = ADS_set<Key, N>::PrivateBucketIterator;
    using Bucket = ADS_set<Key, N>::Bucket;

    Iterator()
        : beginIterator_{BucketIterator(nullptr, 0, 0)}
        , endIterator_{BucketIterator(nullptr, 0, 0)}
        , position_{nullptr}
        , index_{(size_t)-1}
    {

    }

    explicit Iterator(BucketIterator beginIt, BucketIterator endIt, Bucket* position, size_t index)
            : beginIterator_{beginIt}
            , endIterator_{endIt}
            , position_{position}
            , index_{index}
    {

    }

    reference operator*() const
    {
        if(position_ == nullptr)
        {
            throw std::runtime_error("Access beyond iterator");
        }

        const value_type* value = &position_->keys[index_];

        return *value;
    };
    pointer operator->() const
    {
        const value_type* val = &position_->keys[index_];
        return val;
    };

    Iterator &operator++() {
        advanceToNext();
        return *this;
    }

    Iterator operator++(int) {
        Iterator other {*this};
        ++*this;
        return other;
    }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
        return lhs.position_ == rhs.position_ && lhs.index_ == rhs.index_;
    };
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return !(lhs == rhs); };
};

template<typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H