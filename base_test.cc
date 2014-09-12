#include <iostream>

#include "base/basictypes.hh"
#include "unit_testing/gtest-1.7.0/include/gtest/gtest.h"
#include "base/time/time.hh"
#include "base/logging/logging.hh"

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    base::Time time_now = base::Time::Now();
    std::cout << time_now.ToInternalValue() << std::endl;
    char time[80];
    time_now.ToUTCString(time, 80);
    std::cout << time << std::endl;
    base::Time a_time;
    base::Time::FromUTCString(time, &a_time);
    std::cout << a_time.ToInternalValue() << std::endl;
    a_time.ToUTCString(time, 80);
    std::cout << time << std::endl;
    LOG(INFO) << "this is a log test" << std::endl;
    return RUN_ALL_TESTS();
}
