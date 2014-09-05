#include <iostream>
#include "base/basictypes.h"
#include "base/flags.h"

void basictypes_h_test()
{
    COMPILE_ASSERT(4>1, a_4_is_bigger_than_1);
    int i_array[] = {1, 2, 3, 4};
    std::cout << arraysize(i_array) << std::endl;
}

int main()
{
    basictypes_h_test();
}
