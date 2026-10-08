#!/usr/bin/env python
#
# Copyright 2026, Google Inc.
# All rights reserved.
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

"""Tests the configurable RecordProperty() value limit in XML output.

Runs gtest_xml_truncate_test_ with --gtest_output pointing at an XML file in a
temporary directory, and verifies that:
  * by default every recorded value is output in full;
  * when --gtest_property_value_limit is set, values are truncated on complete
    UTF-8 code point boundaries and marked with "...";
  * XML entities produced by escaping are never cut in half;
  * the XML remains parseable.

The test binary itself verifies (and fails the run if broken) that the values
stored internally by RecordProperty() are never modified by output truncation.
"""

import os

from xml.dom import minidom

from googletest.test import gtest_test_utils

GTEST_OUTPUT_FLAG = '--gtest_output'
GTEST_FILTER_FLAG = '--gtest_filter'
GTEST_PROPERTY_VALUE_LIMIT_FLAG = '--gtest_property_value_limit'

GTEST_PROGRAM_NAME = 'gtest_xml_truncate_test_'

TINY_VALUE = '\U0001F600"x&y"'
LONG_EMOJI_VALUE = '\U0001F600' * 20  # 80 UTF-8 bytes
AMP_VALUE = '&' * 10

MARKER = '...'


def _RunXmlOutput(extra_args=None, env=None):
  """Runs the test binary and returns the parsed XML document and its path."""
  xml_path = os.path.join(
      gtest_test_utils.GetTempDir(), 'gtest_xml_truncate_detail.xml'
  )
  if os.path.isfile(xml_path):
    os.remove(xml_path)

  command = [
      gtest_test_utils.GetTestExecutablePath(GTEST_PROGRAM_NAME),
      '%s=xml:%s' % (GTEST_OUTPUT_FLAG, xml_path),
  ]
  if extra_args:
    command.extend(extra_args)

  process = gtest_test_utils.Subprocess(command, env=env)
  assert process.exited, ('test binary did not exit: %s' % process.output)
  assert process.exit_code == 0, (
      'test binary failed with code %s:\n%s'
      % (process.exit_code, process.output)
  )
  assert os.path.isfile(xml_path), 'XML output was not created at %s' % xml_path

  with open(xml_path, 'rb') as xml_file:
    raw_xml = xml_file.read()
  return minidom.parseString(raw_xml), xml_path, raw_xml


def _PropertyValues(document):
  """Returns a {name: value} dict of all recorded XML properties."""
  values = {}
  for property_node in document.getElementsByTagName('property'):
    values[property_node.getAttribute('name')] = property_node.getAttribute(
        'value'
    )
  return values


class GTestXmlPropertyValueLimitTest(gtest_test_utils.TestCase):
  """Verifies property value truncation in the XML report."""

  def testDefaultReportKeepsEverything(self):
    """Without a limit the full value survives XML escaping."""
    document, _, _ = _RunXmlOutput()
    values = _PropertyValues(document)

    # The tiny value with an emoji, quotes and an ampersand is intact.
    self.assertEqual(TINY_VALUE, values['tiny'])
    # The long emoji-only value is output in full as well.
    self.assertEqual(LONG_EMOJI_VALUE, values['long_emoji'])
    self.assertEqual(AMP_VALUE, values['amp'])

  def testTruncationStopsOnCodePointBoundaryAndIsMarked(self):
    """A limit cuts only complete code points and appends the marker."""
    # 80 bytes of emoji plus a 3-byte marker do not fit in 20 bytes; keep
    # only complete 4-byte code points: (20 - 3) // 4 = 4 emojis + "...".
    document, _, raw_xml = _RunXmlOutput(
        ['%s=20' % GTEST_PROPERTY_VALUE_LIMIT_FLAG]
    )
    values = _PropertyValues(document)

    expected_long = '\U0001F600' * 4 + MARKER
    self.assertEqual(expected_long, values['long_emoji'])
    # The value (no XML entities here) never exceeds the configured byte
    # budget and remains valid UTF-8.
    self.assertLessEqual(len(expected_long.encode('utf-8')), 20)
    expected_long.encode('utf-8').decode('utf-8')
    # Parsing already succeeded, which proves the XML and code points are
    # valid; assert the marker is present in the raw report too.
    self.assertIn(b'..."', raw_xml)

    # The tiny value fits within the limit and is left untouched.
    self.assertEqual(TINY_VALUE, values['tiny'])

  def testEntitiesAreNotSplitByTruncation(self):
    """Escaping happens after truncation, so &amp; is never cut."""
    # Limit of 5: content budget is 5 - 3 = 2 bytes, so two '&' are kept and
    # then escaped to two complete "&amp;" entities.
    document, _, raw_xml = _RunXmlOutput(
        ['%s=*AmpersandEntityProperty'
         % GTEST_FILTER_FLAG,
         '%s=5' % GTEST_PROPERTY_VALUE_LIMIT_FLAG]
    )
    values = _PropertyValues(document)

    self.assertEqual('&&' + MARKER, values['amp'])
    # Both ampersands must appear as complete entities in the raw XML, and
    # no half-escaped fragment ("&a" etc. followed by the closing quote) may
    # exist, proving escaping is applied after truncation.
    self.assertIn(b'&amp;&amp;...', raw_xml)
    self.assertNotIn(b'&a"', raw_xml)
    self.assertNotIn(b'&am"', raw_xml)
    self.assertNotIn(b'&amp"', raw_xml)

  def testTinyPropertyAtTinyLimitIsValid(self):
    """A limit smaller than the value still yields parseable XML."""
    document, _, _ = _RunXmlOutput(
        ['%s=*TinyUtf8AndEntitiesProperty'
         % GTEST_FILTER_FLAG,
         '%s=4' % GTEST_PROPERTY_VALUE_LIMIT_FLAG]
    )
    values = _PropertyValues(document)

    # Content budget is a single byte, which is too small for the leading
    # 4-byte emoji code point, so no content byte is kept and only the marker
    # is output. No partial emoji byte reaches the output.
    self.assertEqual(MARKER, values['tiny'])

  def testEnvironmentVariableConfiguresLimit(self):
    """GTEST_PROPERTY_VALUE_LIMIT works just like the command line flag."""
    env = dict(os.environ)
    env['GTEST_PROPERTY_VALUE_LIMIT'] = '20'
    document, _, _ = _RunXmlOutput(env=env)
    values = _PropertyValues(document)

    self.assertEqual('\U0001F600' * 4 + MARKER, values['long_emoji'])
    self.assertEqual(TINY_VALUE, values['tiny'])


if __name__ == '__main__':
  gtest_test_utils.Main()
