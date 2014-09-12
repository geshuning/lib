#include <iostream>
#include <sstream>
#include <string>
#include <locale>
#include <ctime>
#include <iomanip>
#include <functional>

void callback()
{
    std::cout << "callback" << std::endl;
}

int main()
{
    // std::function<void()> c = callback();
    // c();
    constexpr int i = 20;
    const int j = 30;
}
