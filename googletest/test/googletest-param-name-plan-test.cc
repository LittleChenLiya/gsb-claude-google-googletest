// Copyright 2026, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Tests for the explicit parameterized naming plan.
//
// When an instantiation provides a custom name generator (the third
// argument of INSTANTIATE_TEST_SUITE_P), Google Test must:
//   * invoke the name callback exactly once per parameter, no matter how
//     many TEST_P patterns the suite has;
//   * preserve the suite identity (instantiation prefix) and the parameter
//     index handed to the callback;
//   * register every (test pattern, parameter) combination once the whole
//     plan has been validated.
// The default registration mode (no custom name generator) must keep
// working unchanged.

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

constexpr int kPlanParamBase = 7;
const int kNamingPlanParams[] = {kPlanParamBase, kPlanParamBase + 1,
                                 kPlanParamBase + 2};

// Records every invocation of the custom name generator.
struct NameCall {
  int param;
  size_t index;
};

std::vector<NameCall>* NameCalls() {
  static std::vector<NameCall>* calls = new std::vector<NameCall>();
  return calls;
}

std::string CountingNameGenerator(const testing::TestParamInfo<int>& info) {
  NameCalls()->push_back({info.param, info.index});
  return "P" + std::to_string(info.index);
}

class NamingPlanTest : public testing::TestWithParam<int> {};

TEST_P(NamingPlanTest, FirstPattern) {
  // The suite identity (instantiation prefix + suite name) and the
  // parameter-derived suffix must be preserved in the registered name.
  const testing::TestInfo* const info =
      testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ("Plan/NamingPlanTest", info->test_suite_name());
  EXPECT_EQ("FirstPattern/P" + std::to_string(GetParam() - kPlanParamBase),
            info->name());
}

TEST_P(NamingPlanTest, SecondPattern) {}

INSTANTIATE_TEST_SUITE_P(Plan, NamingPlanTest,
                         testing::ValuesIn(kNamingPlanParams),
                         CountingNameGenerator);

// A suite using the default registration mode (no custom name generator).
class DefaultNamingTest : public testing::TestWithParam<int> {};

TEST_P(DefaultNamingTest, Works) {}

INSTANTIATE_TEST_SUITE_P(Default, DefaultNamingTest, testing::Values(1, 2));

const testing::TestSuite* FindSuite(const char* name) {
  const testing::UnitTest* const unit_test =
      testing::UnitTest::GetInstance();
  for (int i = 0; i < unit_test->total_test_suite_count(); ++i) {
    const testing::TestSuite* const candidate = unit_test->GetTestSuite(i);
    if (name == std::string(candidate->name())) return candidate;
  }
  return nullptr;
}

// The name callback must run exactly once per parameter even though the
// suite contains two TEST_P patterns (3 calls for 3 parameters, not 6).
TEST(NamingPlan, NameCallbackRunsOncePerParameter) {
  ASSERT_EQ(3u, NameCalls()->size());
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(kNamingPlanParams[i], (*NameCalls())[i].param)
        << "parameter value at plan position " << i;
    EXPECT_EQ(i, (*NameCalls())[i].index)
        << "parameter index at plan position " << i;
  }
}

// All three parameters of every TEST_P pattern must be registered under
// the suite identity "Plan/NamingPlanTest".
TEST(NamingPlan, AllParametersRegistered) {
  const testing::TestSuite* const suite = FindSuite("Plan/NamingPlanTest");
  ASSERT_NE(nullptr, suite);
  ASSERT_EQ(6, suite->total_test_count());

  std::vector<std::string> names;
  for (int i = 0; i < suite->total_test_count(); ++i) {
    names.push_back(suite->GetTestInfo(i)->name());
  }
  const char* const expected[] = {"FirstPattern/P0", "FirstPattern/P1",
                                  "FirstPattern/P2", "SecondPattern/P0",
                                  "SecondPattern/P1", "SecondPattern/P2"};
  for (const char* name : expected) {
    EXPECT_TRUE(std::find(names.begin(), names.end(), name) != names.end())
        << "missing registered test " << name;
  }
}

// The default registration mode (no custom name generator) must keep
// naming tests by their parameter index.
TEST(NamingPlan, DefaultNamingStillWorks) {
  const testing::TestSuite* const suite = FindSuite("Default/DefaultNamingTest");
  ASSERT_NE(nullptr, suite);
  ASSERT_EQ(2, suite->total_test_count());
  EXPECT_STREQ("Works/0", suite->GetTestInfo(0)->name());
  EXPECT_STREQ("Works/1", suite->GetTestInfo(1)->name());
}

}  // namespace
