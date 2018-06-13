#include <iostream>
#include "ADS_set.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    ADS_set<int>* linearHashTable = new ADS_set<int>({12, 13, 16, 22, 58, 10, 66, 90, 22, 44, 901});

    linearHashTable->dump(std::cout);

    return 0;
}