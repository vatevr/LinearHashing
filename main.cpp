#include <iostream>
#include <algorithm>
#include <iostream>
#include <list>
#include <numeric>
#include <random>
#include <vector>
#include "ADS_set.h"
#include <unordered_map>

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::vector<int> v( 123 );
    std::iota( std::begin( v ), std::end( v ), 1 );
    ADS_set<int>* linearHashTable = new ADS_set<int>(std::begin(v), std::end(v));

    linearHashTable->insertKey(16);
    linearHashTable->insertKey(13);
    linearHashTable->insertKey(21);
    linearHashTable->insertKey(37);
    linearHashTable->insertKey(11);
    linearHashTable->insertKey(10);

    linearHashTable->dump(std::cout);

    std::cout<<std::endl;

    ADS_set<int>* tableCopy = new ADS_set<int>(*linearHashTable);

    for (ADS_set<int>::Iterator it = tableCopy->begin(); it != tableCopy->end() ; it++) {
        std::cout<<*it<<", ";
    }



    return 0;
}