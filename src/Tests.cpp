#include "Tests.h"
#include <iostream>

Tests::Tests()
{
    NCH_FsUtils fsu;
    //FILE* fx = fopen("test.c", "w");
    //fclose(fx);

    printf("Starting tests...\n");

    if(fsu.pathExists("res/test.txt")) {
        printf("Exists.\n");
    } else {
        printf("Doesn't exist.\n");
    }

    auto vec = NCH_FsUtils::getDirContents("..", 0);
    printf("=-=-= Test Result =-=-=\n");
    for(int i = 0; i<vec.size(); i++) {
	printf("%s\n", vec[i].c_str());
    }
}

Tests::~Tests()
{

}