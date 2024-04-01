#include <iostream>

int main(void) {
    int a,b;
    a = 1;
    b = 0;
    for (int i = 0; i < 10; i++) {
        b+=a;
    }
    std::cout << b;
    return 0;
}
