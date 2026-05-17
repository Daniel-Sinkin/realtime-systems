#include "types.hpp"

#include <iostream>

auto main() -> int
{
    static_assert(sizeof(ds_rt::u32) == 4zu);

    std::cout << "hello world\n";
    return 0;
}
