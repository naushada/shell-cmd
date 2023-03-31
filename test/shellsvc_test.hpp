#ifndef __shellsvc_test_hpp__
#define __shellsvc_test_hpp__

#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "shell.hpp"

class ShellSvcTest : public ::testing::Test
{
    public:
        ShellSvcTest(std::string in);
        ~ShellSvcTest() = default;
     
        virtual void SetUp() override;
        virtual void TearDown() override;
        virtual void TestBody() override;
};

#endif /*__shellsvc_test_hpp__*/