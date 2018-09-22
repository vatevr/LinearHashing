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
    struct Element {
        Key element;
        bool used = false;
    };

    struct Bucket {
        Element *elements;
        Bucket *overflowBucket{nullptr};

        Bucket() {
            elements = new Element[N];
        }

        ~Bucket() {
            delete[] elements;

            if (overflowBucket) {
                delete overflowBucket;
            }
        }
    };

    friend class PrivateBucketIterator;
    class PrivateBucketIterator {
    private:
        Bucket* _table;
        size_t _index;
        size_t _size;
    public:
        using value_type = Bucket;
        using reference = value_type &;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;

        PrivateBucketIterator(Bucket* hashTable, size_t index, size_t size)
                : _table{hashTable}, _index{index}, _size{size}
        {}

        PrivateBucketIterator(const PrivateBucketIterator& other)
                : PrivateBucketIterator(other._table, other._index, other._size)
        {}

        reference operator*() const
        {
            if(_index == SIZE_INVALID)
                throw std::runtime_error("Table array exceeded");

            Bucket* bucket = &_table[_index];

            return *bucket;
        };

        pointer operator->() const
        {
            if(_index == SIZE_INVALID)
                throw std::runtime_error("Table array exceeded");

            return &_table[_index];
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

    // Initial size of table 2 ^ d
    size_t nextToSplit{0}, tableSize{0}, d{2}, maxSize;
    // Max load factor of 70%
    float maxLoadFactor{0.7};
    Bucket* table{nullptr};

    using bucketIterator = PrivateBucketIterator;

    void split() {
        Bucket* tmp = new Bucket[maxSize + 1];
        for (size_t i = 0; i < maxSize; ++i) {
            std::swap(tmp[i].elements, table[i].elements);
            std::swap(tmp[i].overflowBucket, table[i].overflowBucket);
            /* key ---> hash ---> 0
             * 0 x-- -> null
             * 1 xxx -> x---
             * 2 --- -> null
             * 3 --- -> null
             * 4 x-- -> null
             */
        }
        delete[] table;
        table = tmp;
        ++maxSize;
    }

    size_t bucketAddress(const Key& key) const {
        size_t n = hasher{}(key);
        if (n % (size_t)(1 << d) >= nextToSplit)
            return n % (size_t)(1 << d);
        else
            return n % (size_t)(1 << (d + 1));
    }

    void rehash(size_t index) {
        Bucket* bucket = &table[index];

        size_t address = index + (1 << d);
        Bucket* bucketToStore = &table[address];
        size_t bucketToStoreIndex = 0;

        while (bucket) {
            for (size_t i = 0; i < N; ++i) {
                if (bucket->elements[i].used && bucketAddress(bucket->elements[i].element) != index) {
                    key_type key = bucket->elements[i].element;
                    bucket->elements[i].used = false;


                    if (bucketToStoreIndex == N) {
                        bucketToStoreIndex = 0;
                        bucketToStore->overflowBucket = new Bucket();
                        bucketToStore = bucketToStore->overflowBucket;
                    }

                    bucketToStore->elements[bucketToStoreIndex].element = key;
                    bucketToStore->elements[bucketToStoreIndex].used = true;
                    ++bucketToStoreIndex;
                }
            }

            bucket = bucket->overflowBucket;
        }
    }

    void insertUnchecked(const key_type &key, bool toSplit = true) {
        size_type address = bucketAddress(key);
        Bucket* bucket = &table[address];
        bool toRehash = false;
        bool inserted = false;

        while(bucket && !inserted) {
            for (size_t i = 0; i < N; ++i) {
                // Should at some point reach here
                if (!bucket->elements[i].used) {
                    bucket->elements[i].element = key;
                    bucket->elements[i].used = true;
                    inserted = true;
                    break;
                }

                if (i == N - 1) {
                    if (!bucket->overflowBucket) {
                        bucket->overflowBucket = new Bucket();
                        toRehash = true;
                    }

                    bucket = bucket->overflowBucket;
                }
            }
        }

        ++tableSize;

        if (toRehash && toSplit) {
            split();

            rehash(nextToSplit++);

            // Splitting is through
            if (1 << d == nextToSplit) {
                ++d;
                nextToSplit = 0;
            }
        }
    }

public:
    ADS_set() {
        maxSize = (size_t)(1<<d);
        table = new Bucket[maxSize];
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
        delete[] table;
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

    size_type size() const { return tableSize; }

    bool empty() const { return !tableSize;};

    // Wie oft gegebene Wert gespeichert ist
    size_type count(const key_type& key) const {
        if (empty()) {
            return 0;
        }


        size_type index = bucketAddress(key);

        Bucket* bucket = &table[index];

        while (bucket) {
            for (size_type i{0}; i < N; ++i) {
                if (bucket->elements[i].used) {
                    if (key_equal{}(key, bucket->elements[i].element)) {
                        return 1;
                    }
                }
            }

            bucket = bucket->overflowBucket;
        }

        return 0;
    };

    iterator find(const key_type& key) const {
        size_t index = bucketAddress(key);
        Bucket* bucket = &table[index];
        while (bucket) {
            for (size_t i = 0; i < N; ++i) {
                if (bucket->elements[i].used && key_equal{}(bucket->elements[i].element, key)) {
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
        std::swap(table, other.table);
        std::swap(d, other.d);
        std::swap(nextToSplit, other.nextToSplit);
        std::swap(tableSize, other.tableSize);
        std::swap(maxSize, other.maxSize);
        std::swap(maxLoadFactor, other.maxLoadFactor);
    }

    void insert(std::initializer_list<key_type> ilist) {
        for (const auto &key: ilist) {
            insert(key);
        }
    }

    std::pair<iterator, bool> insert(const key_type &key) {
        iterator it{find(key)};
        if (it != end()) {
            return {it, false};
        }

        insertUnchecked(key);
        return {iterator{find(key)},true};
    }

    template<typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last ; ++it) {
            insert(*it);
        }
    }

    size_type erase(const key_type &key) {
        size_t index = bucketAddress(key);
        Bucket* bucket = &table[index];

        while (bucket) {
            for (size_t i = 0; i < N; ++i) {
                if (bucket->elements[i].used && key_equal{}(bucket->elements[i].element, key)) {
                    bucket->elements[i].used = false;
                    --tableSize;
                    return 1;
                }
            }

            bucket = bucket->overflowBucket;
        }

        return 0;
    }

    bucketIterator bucketBegin(size_t index) const { return bucketIterator(this->table, index, maxSize); }
    bucketIterator bucketEnd() const { return bucketIterator(this->table, SIZE_INVALID, maxSize); }
    const_iterator begin() const { return const_iterator{bucketBegin(0), bucketEnd(), table, 0}; };
    const_iterator end() const { return const_iterator{bucketEnd(), bucketEnd(), nullptr, SIZE_INVALID}; };

    void dump(std::ostream &o = std::cerr) const {
        for (size_type i{0}; i < maxSize; ++i) {
            Bucket* bucket = &table[i];

            while (bucket != nullptr) {
                for (size_type j{0}; j < N; ++j) {
                    bucket->elements[j].used ? o << bucket->elements[j].element : o << "-";
                    o << " ";
                }

                bucket = bucket->overflowBucket;
            }

            o << "\n";
        }
    };

    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
        if (lhs.tableSize != rhs.tableSize) return false;

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

    void advanceToNext() {
        if (index_ == N) {
            position_ = position_->overflowBucket;
            index_ = 0;
        }

        if (position_) {
            for (size_t i = index_; i < N; ++i) {
                if (position_->elements[i].used) {
                    index_ = i;
                    return;
                }
            }

            position_ = position_->overflowBucket;
            index_ = 0;
            advanceToNext();
        } else {
            if (++beginIterator_ != endIterator_) {
                position_ = &(*beginIterator_);
                index_ = 0;
                advanceToNext();
            } else {
                index_ = (size_t)-1;
            }
        }
    }
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;
    using BucketIterator = ADS_set<Key, N>::PrivateBucketIterator;
    using Bucket = ADS_set<Key, N>::Bucket;

    Iterator(): beginIterator_{BucketIterator(nullptr, 0, 0)}, endIterator_{BucketIterator(nullptr, 0, 0)}, position_{
            nullptr}, index_{(size_t)-1} {

    }

    explicit Iterator(BucketIterator beginIt, BucketIterator endIt, Bucket* position, size_t index)
            : beginIterator_{beginIt}, endIterator_{endIt}, position_{position}, index_{index}
    {
        if(beginIterator_ != endIterator_) {
            advanceToNext();
        }
    }

    reference operator*() const
    {
        if(position_ == nullptr)
        {
            throw std::runtime_error("Access beyond iterator");
        }

        const value_type *value = &position_->elements[index_].element;

        return *value;
    };
    pointer operator->() const
    {
        const value_type *val = &position_->elements[index_].element;
        return val;
    };

    Iterator &operator++() {
        ++index_;
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