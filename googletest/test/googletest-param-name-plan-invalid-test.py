#!/usr/bin/env python
#
# Copyright 2026 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Verifies that a rejected parameterized naming plan registers nothing."""

from googletest.test import gtest_test_utils

binary_name = 'googletest-param-name-plan-invalid-test_'
COMMAND = gtest_test_utils.GetTestExecutablePath(binary_name)


def Assert(condition):
  if not condition:
    raise AssertionError


def Run(mode):
  return gtest_test_utils.Subprocess(
      [COMMAND, '--naming_failure_mode=' + mode])


def AssertPlanRejected(p):
  """A rejected plan aborts the run before any test is registered or run."""
  # The process must not exit successfully (SIGABRT on Unix, a non-zero
  # exit code on Windows).
  Assert(p.terminated_by_signal or p.exit_code != 0)
  # Zero tests registered: no test body may start or pass.
  Assert('[ RUN' not in p.output)
  Assert('[  PASSED  ]' not in p.output)


class GTestParamNamePlanInvalidTest(gtest_test_utils.TestCase):

  def testDuplicateNameReportsBothIndexesAndRegistersNothing(self):
    p = Run('duplicate')
    AssertPlanRejected(p)
    Assert("Duplicate parameterized test name 'alpha'" in p.output)
    # Both conflicting parameter indexes must be reported.
    Assert('indexes 0 and 2' in p.output)

  def testEmptyNameIsExplicitlyRejected(self):
    p = Run('empty')
    AssertPlanRejected(p)
    Assert('empty' in p.output)
    Assert('index 1' in p.output)


if __name__ == '__main__':
  gtest_test_utils.Main()
