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

    std::vector<int> v( 100 );
    std::iota( std::begin( v ), std::end( v ), 1 );

    for (int j = 0; j < 100; j++)
    {

        int i = rand() % 100;
        v[j] = i; //I believe this should be array[j]=i
        std::cout<<v[j]<<", ";
    }

    std::cout<<std::endl;

    ADS_set<int>* linearHashTable = new ADS_set<int>(std::begin( v ), std::end( v ));

    linearHashTable->dump(std::cout);

    std::cout<<std::endl;

    for (ADS_set<int>::Iterator it = linearHashTable->begin(); it != linearHashTable->end() ; it++) {
        std::cout<<*it<<", ";
    }

    return 0;
}