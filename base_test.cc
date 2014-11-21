#include <iostream>
#include <functional>
#include "base/basictypes.hh"
#include "unit_testing/gtest-1.7.0/include/gtest/gtest.h"
#include "base/time/time.hh"
#include "base/logging/logging.hh"
#include "base/strings/string_number_conversion.hh"
#include "base/threading/thread.hh"

LOG_DEFINE_THIS_MODULE(main);

void log_test()
{
    INFO_LOG_TO_FILE("main.log");
    LOG_INFO() << "hello world";
}

void print_num(int i, int j)
{
    std::cout << "function " << i << " " << j << "\n";
}

int main(int argc, char **argv)
{
    log_test();
    return 0;
    testing::InitGoogleTest(&argc, argv);
    base::Time time_now = base::Time::Now();
    std::cout << time_now.ToInternalValue() << std::endl;
    // char time[80];
    std::string time = base::TimeConverter::Time2String(time_now, false);
    std::cout << time << std::endl;
    base::Time a_time;
    a_time = base::TimeConverter::String2Time(time.c_str(), false);
    std::cout << a_time.ToInternalValue() << std::endl;
    time = base::TimeConverter::Time2String(a_time, true);
    std::cout << time << std::endl;
    LOG(INFO) << "this is a log test" << std::endl;

    int i = -100;
    std::string i_str = base::IntToString(i);
    std::cout << i_str << std::endl;

    std::function<void(int, int)> f = print_num;
    f(i, i);
    return RUN_ALL_TESTS();
}
