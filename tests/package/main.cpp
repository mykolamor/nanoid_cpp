#include "nanoid/nanoid.h"
#include <iostream>

int main()
{
    const std::string id = nanoid::generate();
    std::cout << id << '\n';
    return id.size() == 21 ? 0 : 1;
}
