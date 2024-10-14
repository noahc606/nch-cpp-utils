#include "Tests.h"
#include <iostream>
#include <nch/cpp-utils/arraylist.h>
#include <nch/cpp-utils/noah-alloc-table.h>
#include <nch/cpp-utils/file-utils.h>
#include <nch/cpp-utils/string-utils.h>
#include <sstream>

Tests::Tests()
{
    nch::FsUtils fsu;

    nch::NoahAllocTable nat("tests/nch_nat_test_1");

    std::vector<unsigned char> data = { };
    //0b01010101, 0b11111111, 0b11111111, 0b11111111, 0b00000001
    for(int i = 0; i<257; i++) {
        data.push_back(0b10101011);
    }
    nat.save("test_e", data);

    std::stringstream info;
    nat.putInfo(info);
    printf("%s\n", info.str().c_str());

    //unsigned char x = 0b11111111;
    //
    //nat.alloc("test label", &data[0], data.size());
    
    auto nv = nat.load("test_e");
    nat.close();


    return;

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