#include <iostream>
//#include "ADS_set.h"
#include <unordered_map>

int main() {
    std::cout << "Hello, World!" << std::endl;

//    ADS_set<int>* linearHashTable = new ADS_set<int>({8, 17, 34, 25, 50, 2, 28, 5, 55});
//
//    linearHashTable->insertKey(16);
//    linearHashTable->insertKey(13);
//    linearHashTable->insertKey(21);
//    linearHashTable->insertKey(37);
//
//    linearHashTable->dump(std::cout);

    std::unordered_map<std::string, std::string> u = {
            {"RED","#FF0000"},
            {"GREEN","#00FF00"},
            {"BLUE","#0000FF"}
    };

    std::cout<< u.end()->first << ", " << u.end()->second;

    return 0;
}