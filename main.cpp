#include <iostream>
#include "ADS_set.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    ADS_set<int>* linearHashTable = new ADS_set<int>({8, 17, 34, 25, 50, 2, 28, 5, 55});

    linearHashTable->insertKey(16);
    linearHashTable->insertKey(13);
    linearHashTable->insertKey(21);
    linearHashTable->insertKey(37);

    linearHashTable->dump(std::cout);

    return 0;
}