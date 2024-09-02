#include "Tests.h"
#include <iostream>
#include "nch/cpp-utils/data/ArrayList.h"

Tests::Tests()
{
    nch::FsUtils fsu;
    //FILE* fx = fopen("test.c", "w");
    //fclose(fx);

    /*
    printf("Starting tests...\n");

    if(fsu.pathExists("res/test.txt")) {
        printf("Exists.\n");
    } else {
        printf("Doesn't exist.\n");
    }

    auto vec = nch::FsUtils::getDirContents("..", 0);
    printf("=-=-= Test Result =-=-=\n");
    for(int i = 0; i<vec.size(); i++) {
	    printf("%s\n", vec[i].c_str());
    }*/

   nch::ArrayList<int> al;

   al.pushBack(4);
   al.pushBack(12);
   al.pushBack(44);
   al.pushBack(123);
   al.pushBack(1234);
   al.pushBack(12345);
   al.pushBack(123456);

   al.eraseMultiple({1, 2, 3});

   printf("obj: %d\n", al.at(1));

}

Tests::~Tests()
{

}