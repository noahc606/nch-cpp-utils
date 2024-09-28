#include "Tests.h"
#include <iostream>
#include "nch/cpp-utils/arraylist.h"

Tests::Tests()
{
    nch::FsUtils fsu;
    //FILE* fx = fopen("test.c", "w");
    //fclose(fx);

    
    printf("Starting tests...\n");

    auto vec = nch::FsUtils::getDirContents("");
    printf("=-=-= Test Result =-=-=\n");
    for(int i = 0; i<vec.size(); i++) {
	    printf("%s\n", vec[i].c_str());
    }

    printf("%s\n", nch::FsUtils::getPathWithInferredExtension("nchutils-test-win-x86_64").c_str());

    nch::ArrayList<int> al;

    al.pushBack(4);
    al.pushBack(12);
    al.pushBack(44);
    al.pushBack(123);
    al.pushBack(1234);
    al.pushBack(12345);
    al.pushBack(123456);

    al.eraseMultiple({1, 2, 3});

   //printf("obj: %d\n", al.at(1));

}

Tests::~Tests()
{

}