# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import make_runtime_features_utilities as util


def _feature(name, depends_on=[], implied_by=[]):
    return {
        'name': name,
        'depends_on': depends_on,
        'implied_by': implied_by,
    }


class MakeRuntimeFeaturesUtilitiesTest(unittest.TestCase):
    def test_cycle(self):
        # Cycle: 'c' => 'd' => 'e' => 'c'
        with self.assertRaisesRegexp(
                AssertionError, 'Cycle found in depends_on/implied_by graph'):
            util.validate_runtime_features_graph([
                _feature('a', depends_on=['b']),
                _feature('b'),
                _feature('c', implied_by=['a', 'd']),
                _feature('d', depends_on=['e']),
                _feature('e', implied_by=['c'])
            ])

    def test_bad_dependency(self):
        with self.assertRaisesRegexp(AssertionError,
                                     'a: Depends on non-existent-feature: x'):
            util.validate_runtime_features_graph([_feature('a', depends_on=['x'])])

    def test_bad_implication(self):
        with self.assertRaisesRegexp(AssertionError,
                                     'a: Implied by non-existent-feature: x'):
            util.validate_runtime_features_graph([_feature('a', implied_by=['x'])])

    def test_both_dependency_and_implication(self):
        with self.assertRaisesRegexp(
                AssertionError,
                'c: Only one of implied_by and depends_on is allowed'):
            util.validate_runtime_features_graph([
                _feature('a'),
                _feature('b'),
                _feature('c', depends_on=['a'], implied_by=['b'])
            ])


if __name__ == "__main__":
    unittest.main()
