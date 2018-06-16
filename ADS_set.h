#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 3>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = key_type&;
    using const_reference = const key_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    using key_equal = std::equal_to<key_type>; // Hashing
    using hasher = std::hash<key_type>;        // Hashing

private:
    enum class Mode{ free, used };
    static const size_t OVERFLOW_BUCKET_SIZE = N;

    struct OverflowBucket {
        Key* overflowElements = new Key[OVERFLOW_BUCKET_SIZE]();
        Mode overflowMode[OVERFLOW_BUCKET_SIZE];
        OverflowBucket *nextOverflow = nullptr;

        OverflowBucket() {
            for (size_type i{0}; i < OVERFLOW_BUCKET_SIZE; ++i) {
                overflowMode[i] = Mode::free;
            }
        }

        ~OverflowBucket() {
            if (nextOverflow) delete nextOverflow;
        }

        void dump(std::ostream &o) {
            for (size_type j{0}; j < N; ++j) {
                switch (overflowMode[j]) {
                    case Mode::used:
                        o << overflowElements[j];
                        break;
                    case Mode::free:
                        o << "-";
                        break;
                }
                o << " ";
            }
        }
    };

    struct Bucket {
        Key* bucketElements = new Key[N]();
        Mode bucketMode[N];
        OverflowBucket *overflowBucket = nullptr;

        Bucket() {
            for (size_type i{0}; i < N; ++i) {
                bucketMode[i] = Mode::free;
            }
        }

        void dump(std::ostream &o) {
            for (size_type j{0}; j < N; ++j) {
                switch (bucketMode[j]) {
                    case Mode::used:
                        o << bucketElements[j];
                        break;
                    case Mode::free:
                        o << "-";
                        break;
                }
                o << " ";
            }

            if (overflowBucket) {
                o << "--------> ";
                overflowBucket->dump(o);
            }
        }

        ~Bucket() {
            // TODO
        }
    };

    friend class PrivateBucketIterator;

    template <typename Key, size_t N = 3>
    class PrivateBucketIterator {
    private:
        ADS_set<Key, N>* hashTable_;
        size_t index_;

    public:
        using value_type = Bucket;
        using difference_type = std::ptrdiff_t;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;


        PrivateBucketIterator(ADS_set<Key, N>* hashTable, size_t index)
        : hashTable_(hashTable), index_(index)
        {

        }

        PrivateBucketIterator(const PrivateBucketIterator<Key, N>& other)
                : PrivateBucketIterator(other.hashTable_, other.index_)
        {

        }

        reference operator*() const
        {
            if(index_ == -1)
                throw std::runtime_error("Table array exceeded");

            return *hashTable_[index_];
        };

        pointer operator->() const
        {
            if(index_ == -1)
                throw std::runtime_error("Table array exceeded");

            return &hashTable_[index_];
        };

        PrivateBucketIterator& operator++()
        {
            if(index_ == -1)
                throw std::runtime_error("Table array exceeded");

            if(++index_ == N)
                index_ = -1;

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
            return me.hashTable_ == other.hashTable_
                    && me.index_ == other.index_;
        };

        friend bool operator!=(const PrivateBucketIterator& me, const PrivateBucketIterator& other)
        {
            return !(me == other);
        };
    };

    using bucketIterator = PrivateBucketIterator<Key, N>;

    bucketIterator bucketBegin()
    {
        return bucketIterator{this, 0};
    }

    bucketIterator bucketEnd()
    {
        return bucketIterator{this, -1};
    }

    size_type bucketsSize;
    size_type d{3};
    size_type nextToSplit{0};
    size_type tableSize{0};

    Bucket* table {nullptr};
    // 70% is optimal value
    float maxLoadFactor{0.7};

    size_type hashIndex(const key_type &key) const { return hasher{}(key) % (size_type)pow(2, d); }
    size_type nextIndex(const key_type &key) const { return hasher{}(key) % (size_type)pow(2, d + 1); }

    // Baut tabelle die genau die Größe tableSize hat
    void rehash(size_type index);
    void rehashOverflow(OverflowBucket*);

    void destructOverflow(int index) {
        delete table[index].overflowBucket;
        table[index].overflowBucket = nullptr;
    }

    bool isSplittingComplete() { return nextToSplit + 1 == pow(2, d); };
    void nextRound() {
        nextToSplit = 0;
        ++d;
    };

    void split();
    void addToOverflow(Key, OverflowBucket*);

public:
    ADS_set() {
        bucketsSize = pow(2, d);
        table = new Bucket[bucketsSize];
    }

    ADS_set(std::initializer_list<key_type> ilist): ADS_set{} { insert(ilist); };
    template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{} { insert(first, last); }
    ADS_set(const ADS_set&) { throw std::runtime_error("Not implemented!"); }
    ~ADS_set() {
        for(size_t i = 0; i < bucketsSize; ++i) {
            if(table[i].overflowBucket) { destructOverflow(i); }
        }

        delete[] table;
    }

    ADS_set& operator=(const ADS_set& ) { throw std::runtime_error("Not implemented!"); };
    ADS_set& operator=(std::initializer_list<key_type> ) { throw std::runtime_error("Not implemented!"); };

    size_type size() const { return tableSize; };
    bool empty() const {
        return !size();
    };

    // Wie oft gegebene Wert gespeichert ist
    size_type count(const key_type& key) const;
    iterator find(const key_type& ) const { throw std::runtime_error("Not implemented!"); };

    void clear() { throw std::runtime_error("Not implemented!"); };
    void swap(ADS_set& ) { throw std::runtime_error("Not implemented!"); };

    void insert(std::initializer_list<key_type> ilist);
    std::pair<iterator,bool> insert(const key_type& ) { throw std::runtime_error("Not implemented!"); };
    template<typename InputIt> void insert(InputIt first, InputIt last);

    size_type erase(const key_type& ){ throw std::runtime_error("Not implemented!"); };

    const_iterator begin() const { throw std::runtime_error("Not implemented!"); };
    const_iterator end() const { throw std::runtime_error("Not implemented!"); };

    void dump(std::ostream& o = std::cerr) const;
    Bucket *insertKey(const key_type &key);

    friend bool operator==(const ADS_set& , const ADS_set& ) { throw std::runtime_error("Not implemented!"); };
    friend bool operator!=(const ADS_set& , const ADS_set& ) { throw std::runtime_error("Not implemented!"); };
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
    ADS_set<Key, N>::Bucket* position_;
    ADS_set<Key, N>::OverflowBucket* overflowBucket_;
    size_t index_;
    // size of table
    size_t bucketCount_;

    void advanceToUsed() {

    }
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;
    using BucketMode = ADS_set<Key, N>::Mode;
    using Bucket = ADS_set<Key, N>::Bucket;

    explicit Iterator(size_t bucketCount, size_t index, Bucket* position)
    :position_{position}, index_{index}, bucketCount_{bucketCount}
    {

    };
    reference operator*() const { throw std::runtime_error("Not implemented!"); };
    pointer operator->() const { throw std::runtime_error("Not implemented!"); };
    Iterator& operator++() {  };
    Iterator operator++(int) { throw std::runtime_error("Not implemented!"); };
    friend bool operator==(const Iterator& , const Iterator& ) { throw std::runtime_error("Not implemented!"); };
    friend bool operator!=(const Iterator& , const Iterator& ) { throw std::runtime_error("Not implemented!"); };
};

template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs) { lhs.swap(rhs); }

template <typename Key, size_t N>
typename ADS_set<Key,N>::Bucket *ADS_set<Key, N>::insertKey(const key_type &key) {
    if (empty()) split();

    size_type index{hashIndex(key)};

    for (size_type i = 0; i < N; ++i) {
        if (table[index].bucketMode[i] == Mode::free) {
            table[index].bucketElements[i] = key;
            table[index].bucketMode[i] = Mode::used;
            break;
        }

        if (i == N - 1) {
            if (!table[index].overflowBucket) {
                table[index].overflowBucket = new OverflowBucket();
                addToOverflow(key, table[index].overflowBucket);

                split();
                rehash(nextToSplit);

                // Splitting einmal durch
                if (pow(2, d) == nextToSplit) {
                    d++;
                    nextToSplit = 0;
                } else {
                    nextToSplit++;
                }
            } else {
                addToOverflow(key, table[index].overflowBucket);
            }

        }
    }

    tableSize++;
    return table + index;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::split() {
    Bucket* tmp = new Bucket[size()+1];
    std::copy(table, table + size(), tmp);
    delete[] table;
    table = tmp;
    ++bucketsSize;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::rehash(size_type index) {
    bool rehashed = false;
    for (size_t i = 0; i < N; ++i) {
        if (hashIndex(table[index].bucketElements[i]) != nextIndex(table[index].bucketElements[i]) && table[index].bucketMode[i] == Mode::used) {
            for (size_type j = 0; j < N && !rehashed; ++j) {
                if (table[bucketsSize - 1].bucketMode[j] == Mode::free) {
                    table[bucketsSize - 1].bucketElements[j] = table[index].bucketElements[i];
                    table[bucketsSize - 1].bucketMode[j] = Mode::used;
                    table[index].bucketMode[i] = Mode::free;

                    rehashed = true;
                }
            }
        }

        rehashed = false;
    }

    if (table[index].overflowBucket) {
        rehashOverflow(table[index].overflowBucket);
    }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::rehashOverflow(OverflowBucket* overFlowBucket) {
    size_t i = 0;
    bool rehashed = false;
    while(i < (OVERFLOW_BUCKET_SIZE)){
        if(hashIndex(overFlowBucket->overflowElements[i]) != nextIndex(overFlowBucket->overflowElements[i]) && overFlowBucket->overflowMode[i] == Mode::used){
            for(size_t j=0; j < N && !rehashed; ++j){
                if(table[bucketsSize - 1].bucketMode[j] == Mode::free){
                    table[bucketsSize - 1].bucketElements[j] = overFlowBucket->overflowElements[i];
                    table[bucketsSize - 1].bucketMode[j] = Mode::used;

                    overFlowBucket->overflowMode[i] = Mode::free;
                    rehashed = true;
                } else if(j == N - 1 && table[bucketsSize - 1].overflowBucket == nullptr){
                    table[bucketsSize-1].overflowBucket = new OverflowBucket();
                    addToOverflow(overFlowBucket->overflowElements[i], table[bucketsSize - 1].overflowBucket);

                    overFlowBucket->overflowMode[i] = Mode::free;
                    rehashed = true;
                }else if(j == N-1 && table[bucketsSize - 1].overflowBucket != nullptr){
                    addToOverflow(overFlowBucket->overflowElements[i], table[bucketsSize-1].overflowBucket);

                    overFlowBucket->overflowMode[i] = Mode::free;
                    rehashed = true;
                }
            }
            rehashed = false;
            ++i;
        } else {
            ++i;
        }

        if(overFlowBucket->nextOverflow){
            overFlowBucket = overFlowBucket->nextOverflow;
            i = 0;
        }
    }
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::count(const key_type& key) const {
    if (empty()) {
        return 0;
    }


    size_type index = hashIndex(key);
    if(index < nextToSplit) index = nextIndex(key);

    for (size_type i{0}; i < N; ++i) {
        if (table[index].bucketMode[i] == Mode::used) {
            if (key_equal{}(key, table[index].bucketElements[i])) {
                return 1;
            }
        }
    }

    if (table[index].overflowBucket) {
        for (size_type i{0}; i < OVERFLOW_BUCKET_SIZE; ++i) {
            if (key_equal{}(key, table[index].overflowBucket->overflowElements[i])) {
                return 1;
            }
        }
    }

    return 0;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::insert(std::initializer_list<key_type> ilist) {
    for (const auto &key: ilist) {
        insertKey(key);
    }
}

// Optimierungsmöglichkeiten
template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
    for (auto it = first; it != last ; ++it) {
        insertKey(*it);
    }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::addToOverflow(Key key, OverflowBucket *overflowBucket) {
    size_t i = 0;
    while(i < OVERFLOW_BUCKET_SIZE){
        if(overflowBucket->overflowMode[i] == Mode::free){
            overflowBucket->overflowElements[i] = key;
            overflowBucket->overflowMode[i] = Mode::used;
            break;
        }else if(i == (N/2)-1 && !overflowBucket->nextOverflow){
            overflowBucket->nextOverflow = new OverflowBucket();
            overflowBucket = overflowBucket->nextOverflow;
            split();
            rehash(nextToSplit);

            if (isSplittingComplete()) {
                nextRound();
            } else {
                ++nextToSplit;
            }

            i = 0;
        }else if(i == (N/2)-1 && overflowBucket->nextOverflow){
            overflowBucket = overflowBucket->nextOverflow;
            i = 0;
        }else{
            ++i;
        }
    }
}
template<typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream &o) const {
    for (size_type i{0}; i < size(); ++i) {
        table[i].dump(o);

        o << "\n";
    }
}

#endif // ADS_SET_H