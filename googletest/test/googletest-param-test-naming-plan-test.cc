// Copyright 2026 Google Inc.
// All Rights Reserved.
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

// Verifies the explicit parameterized naming plan used by
// INSTANTIATE_TEST_SUITE_P_WITH_NAME_PLAN:
//
//  * All parameter names are generated (one name-generator call per
//    parameter) and validated before any test is registered.
//  * A legal plan registers every parameter of every TEST_P, preserving
//    the test suite identity and the parameter indices.
//  * A plan whose third name duplicates the first one registers zero
//    tests, reports both conflicting parameter indices, and never runs
//    any test body of the offending instantiation.
//  * An empty generated name is explicitly rejected.
//
// This binary is a targeted, single-process test: run it directly (or via
// "ctest -j1 -R googletest-param-test-naming-plan-test"). It verifies its
// own expectations in main() and exits with 0 only if all of them hold.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

// Records every call of the counting name generator.
int g_name_calls = 0;
std::vector<size_t> g_name_indices;

// Records the parameter of every executed test body.
std::vector<std::string> g_body_params;

// Counts executed bodies of the empty-name suite.
int g_empty_body_runs = 0;

class NamingPlanTest : public testing::TestWithParam<std::string> {};

// Names each parameter after its value and counts the calls, so the test
// can verify the generator runs exactly once per parameter.
std::string CountingParamName(
    const testing::TestParamInfo<std::string>& info) {
  ++g_name_calls;
  g_name_indices.push_back(info.index);
  return info.param;
}

TEST_P(NamingPlanTest, BodyA) { g_body_params.push_back(GetParam()); }
TEST_P(NamingPlanTest, BodyB) { g_body_params.push_back(GetParam()); }

// Legal plan: three unique names, all of them must be registered.
INSTANTIATE_TEST_SUITE_P_WITH_NAME_PLAN(Good, NamingPlanTest,
                                        testing::Values("a", "b", "c"),
                                        CountingParamName);

// Invalid plan: the third parameter name duplicates the first one. The
// conflict is only visible after the whole sequence is generated; nothing
// may be registered for this instantiation.
INSTANTIATE_TEST_SUITE_P_WITH_NAME_PLAN(Dup, NamingPlanTest,
                                        testing::Values("x", "y", "x"),
                                        CountingParamName);

class EmptyNameTest : public testing::TestWithParam<int> {};

// Always produces the empty name, which must be explicitly rejected.
std::string EmptyParamName(const testing::TestParamInfo<int>&) { return ""; }

TEST_P(EmptyNameTest, Body) { ++g_empty_body_runs; }

INSTANTIATE_TEST_SUITE_P_WITH_NAME_PLAN(Empty, EmptyNameTest,
                                        testing::Values(1, 2), EmptyParamName);

bool g_ok = true;

void Check(bool condition, const std::string& what) {
  if (condition) {
    std::cout << "[  OK  ] " << what << std::endl;
  } else {
    std::cerr << "[FAILED] " << what << std::endl;
    g_ok = false;
  }
}

const testing::TestSuite* FindTestSuite(const std::string& name) {
  const testing::UnitTest* unit_test = testing::UnitTest::GetInstance();
  for (int i = 0; i < unit_test->total_test_suite_count(); ++i) {
    const testing::TestSuite* test_suite = unit_test->GetTestSuite(i);
    if (name == test_suite->name()) return test_suite;
  }
  return nullptr;
}

std::vector<std::string> TestNamesOf(const testing::TestSuite* test_suite) {
  std::vector<std::string> names;
  for (int i = 0; i < test_suite->total_test_count(); ++i) {
    names.push_back(test_suite->GetTestInfo(i)->name());
  }
  std::sort(names.begin(), names.end());
  return names;
}

// Returns the concatenated failure messages of the given test, or an empty
// string if it does not exist.
std::string FailureSummaryOf(const testing::TestSuite* test_suite,
                             const std::string& test_name) {
  if (test_suite == nullptr) return "";
  for (int i = 0; i < test_suite->total_test_count(); ++i) {
    const testing::TestInfo* test_info = test_suite->GetTestInfo(i);
    if (test_name != test_info->name()) continue;
    std::string summary;
    const testing::TestResult& result = *test_info->result();
    for (int j = 0; j < result.total_part_count(); ++j) {
      summary += result.GetTestPartResult(j).summary();
      summary += "\n";
    }
    return summary;
  }
  return "";
}

}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  const int run_result = RUN_ALL_TESTS();

  // A legal plan registers every parameter of every TEST_P, preserving the
  // test suite identity in the registered test names.
  const testing::TestSuite* good = FindTestSuite("Good/NamingPlanTest");
  Check(good != nullptr, "legal plan registers the Good/NamingPlanTest suite");
  if (good != nullptr) {
    Check(TestNamesOf(good) ==
              std::vector<std::string>({"BodyA/a", "BodyA/b", "BodyA/c",
                                        "BodyB/a", "BodyB/b", "BodyB/c"}),
          "all three legal parameters are registered for both TEST_Ps");
  }

  // The name generator ran exactly once per parameter and instantiation,
  // with the parameter indices 0, 1, 2 preserved - not once per TEST_P.
  Check(g_name_calls == 6,
        "name generator called exactly once per parameter (3+3, not per "
        "TEST_P)");
  Check(g_name_indices == std::vector<size_t>({0, 1, 2, 0, 1, 2}),
        "parameter indices 0, 1, 2 are preserved for each instantiation");

  // The duplicate plan registered zero tests: no Dup/NamingPlanTest suite
  // exists, not even a partially registered one.
  Check(FindTestSuite("Dup/NamingPlanTest") == nullptr,
        "duplicate plan registers zero tests (no partial registration)");

  // No test body of the invalid instantiations ever ran; only the legal
  // parameters a, b, c executed (once in BodyA and once in BodyB).
  std::vector<std::string> body_params = g_body_params;
  std::sort(body_params.begin(), body_params.end());
  Check(body_params ==
            std::vector<std::string>({"a", "a", "b", "b", "c", "c"}),
        "only the legal test bodies ran");
  Check(g_empty_body_runs == 0, "empty-name test bodies never ran");

  // The empty name is explicitly rejected and registers zero tests.
  Check(FindTestSuite("Empty/EmptyNameTest") == nullptr,
        "empty name registers zero tests");

  // Both invalid plans surface as synthetic test failures that carry the
  // test suite identity, so the problems cannot go unnoticed.
  const testing::TestSuite* verification =
      FindTestSuite("GoogleTestVerification");
  Check(verification != nullptr,
        "invalid plans are reported in GoogleTestVerification");
  const std::string dup_summary = FailureSummaryOf(
      verification, "InvalidParameterizedTestNamePlan<Dup/NamingPlanTest>");
  Check(dup_summary.find("Duplicate parameterized test name 'x'") !=
                std::string::npos &&
            dup_summary.find("indices 0 and 2") != std::string::npos,
        "duplicate plan reports both parameter indices (0 and 2)");
  const std::string empty_summary = FailureSummaryOf(
      verification, "InvalidParameterizedTestNamePlan<Empty/EmptyNameTest>");
  Check(empty_summary.find("is empty, which is not allowed") !=
            std::string::npos,
        "empty name is explicitly rejected");
  Check(run_result != 0, "the test run reports the invalid plans as failures");

  std::cout << (g_ok ? "ALL NAMING PLAN CHECKS PASSED"
                     : "NAMING PLAN CHECKS FAILED")
            << std::endl;
  return g_ok ? 0 : 1;
}
