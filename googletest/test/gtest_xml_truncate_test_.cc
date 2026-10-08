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

// Test binary for the configurable RecordProperty() value output limit.
//
// Records properties containing multi-byte UTF-8 code points, quotes and
// ampersands, and verifies in-process that the value stored internally is
// always the full value the test recorded, regardless of any output limit.
// The XML produced by this binary is checked by gtest_xml_truncate_test.py.
// Don't run this program directly.

#include <string>

#include "gtest/gtest.h"

using ::testing::UnitTest;

namespace {

// Returns the recorded value of the named property on the current test
// result, or an empty string if no such property was recorded.
std::string GetRecordedValue(const std::string& key) {
  const testing::TestResult* const result =
      UnitTest::GetInstance()->current_test_info()->result();
  for (int i = 0; i < result->test_property_count(); ++i) {
    const testing::TestProperty& property = result->GetTestProperty(i);
    if (property.key() == key) {
      return property.value();
    }
  }
  return "<missing>";
}

// A tiny value that combines a 4-byte UTF-8 code point (an emoji), double
// quotes and an ampersand.
const char kTinyValue[] = "\xF0\x9F\x98\x80\"x&y\"";  // 😀"x&y"

// A longer value made entirely of 4-byte emoji code points.
std::string MakeLongEmojiValue() {
  static const char kGrinningFace[] = "\xF0\x9F\x98\x80";  // U+1F600 😀
  std::string value;
  for (int i = 0; i < 20; ++i) {
    value += kGrinningFace;
  }
  return value;
}

// A value of ampersands: if truncation were applied after XML escaping, an
// "&amp;" entity could be cut in half and produce invalid XML.
const std::string kAmpersandValue(10, '&');

TEST(PropertyTruncationTest, TinyUtf8AndEntitiesProperty) {
  RecordProperty("tiny", kTinyValue);
  EXPECT_EQ(kTinyValue, GetRecordedValue("tiny"));
}

TEST(PropertyTruncationTest, LongEmojiProperty) {
  const std::string long_emoji_value = MakeLongEmojiValue();
  RecordProperty("long_emoji", long_emoji_value);
  EXPECT_EQ(80, long_emoji_value.size());
  EXPECT_EQ(long_emoji_value, GetRecordedValue("long_emoji"));
}

TEST(PropertyTruncationTest, AmpersandEntityProperty) {
  RecordProperty("amp", kAmpersandValue);
  EXPECT_EQ(kAmpersandValue, GetRecordedValue("amp"));
}

}  // namespace
