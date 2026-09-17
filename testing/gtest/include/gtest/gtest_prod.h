// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_GTEST_INCLUDE_GTEST_GTEST_PROD_H_
#define TESTING_GTEST_INCLUDE_GTEST_GTEST_PROD_H_

// In this tree, Google Test is not built as part of the engine. Production
// code only uses FRIEND_TEST from gtest_prod.h to declare unit-test friends.
#ifndef FRIEND_TEST
#define FRIEND_TEST(test_case_name, test_name) \
  friend class test_case_name##_##test_name##_Test
#endif

#endif  // TESTING_GTEST_INCLUDE_GTEST_GTEST_PROD_H_
