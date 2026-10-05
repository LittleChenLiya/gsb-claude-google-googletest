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

// Helper binary for googletest-param-name-plan-invalid-test.py.
//
// Exercises the rejection paths of the explicit parameterized naming plan:
// the whole plan is validated before anything is registered, so a bad name
// must leave zero registered tests and must never run a test body.
//
// The failure mode is selected with --naming_failure_mode=<mode>:
//   duplicate - the third parameter's name repeats the first one;
//   empty     - the second parameter's name is an empty string.

#include <string>

#include "gtest/gtest.h"

namespace {

std::string* FailureMode() {
  static std::string* mode = new std::string("duplicate");
  return mode;
}

class DummyTest : public ::testing::TestWithParam<int> {};

std::string NameForParam(const testing::TestParamInfo<int>& info) {
  if (*FailureMode() == "empty") {
    // The second parameter gets an explicitly rejected empty name.
    return info.index == 1 ? "" : "ok";
  }
  // "duplicate" mode: the third parameter reuses the first parameter's name.
  return info.index == 0 ? "alpha" : (info.index == 1 ? "beta" : "alpha");
}

TEST_P(DummyTest, NeverRuns) {
  GTEST_FAIL() << "A rejected naming plan must not execute any test body.";
}

INSTANTIATE_TEST_SUITE_P(BadPlan, DummyTest, ::testing::Values(10, 20, 30),
                         NameForParam);

}  // namespace

int main(int argc, char** argv) {
  const std::string prefix = "--naming_failure_mode=";
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg.compare(0, prefix.size(), prefix) == 0)
      *FailureMode() = arg.substr(prefix.size());
  }
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
