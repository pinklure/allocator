//
// Created by PinkLure on 9/6/2022.
//

#include "../cross_alloc.h"


int main() {
#define Alloc(size) CrossAlloc::alloc(size);  CrossAlloc::visualize()
#define Dealloc(ptr) CrossAlloc::dealloc(ptr); CrossAlloc::visualize()
    auto b = Alloc(331);
    auto a = Alloc(124);
    Dealloc(a);
    auto e = Alloc(1025);
    auto c = Alloc(854);
    auto d = Alloc(532);
    Dealloc(d);
    Dealloc(e);
    Dealloc(b);
    auto f = Alloc(7922);
    auto g = Alloc(9012);
#undef Dealloc
#undef Alloc
}