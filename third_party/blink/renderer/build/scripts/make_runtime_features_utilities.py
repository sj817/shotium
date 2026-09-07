# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def _error_message(message, feature, other_feature=None):
    message = 'runtime_enabled_features.json5: {}: {}'.format(feature, message)
    if other_feature:
        message += ': {}'.format(other_feature)
    return message


def validate_runtime_features_graph(features):
    """
    Raises AssertionError when sanity check failed.
    @param features: dictionaries with name, depends_on and implied_by keys.
    @returns None
    """
    feature_pool = {str(f['name']) for f in features}
    for f in features:
        assert not f['implied_by'] or not f['depends_on'], _error_message(
            'Only one of implied_by and depends_on is allowed', f['name'])
        for d in f['depends_on']:
            assert d in feature_pool, _error_message(
                'Depends on non-existent-feature', f['name'], d)
        for i in f['implied_by']:
            assert i in feature_pool, _error_message(
                'Implied by non-existent-feature', f['name'], i)

    graph = {
        str(feature['name']): feature['depends_on'] + feature['implied_by']
        for feature in features
    }
    path = set()

    def has_cycle(vertex):
        path.add(vertex)
        for neighbor in graph[vertex]:
            if neighbor in path or has_cycle(neighbor):
                return True
        path.remove(vertex)
        return False

    for f in features:
        assert not has_cycle(str(f['name'])), _error_message(
            'Cycle found in depends_on/implied_by graph', f['name'])


def browser_read_access(features):
    return [
        f for f in features if f['browser_process_read_access']
        or f['browser_process_read_write_access']
    ]

def browser_write_access(features):
    return [f for f in features if f['browser_process_read_write_access']]

def overridable_features(features):
    """
    Returns a deduplicate list of features that runtime feature state needs to
    keep track of (see runtime_feature_state_override_context).

    These features accept explicit context overrides independently of the
    process-wide runtime flag.
    """
    feature_list = browser_read_access(features)
    seen = set()
    final_list = []
    for f in feature_list:
        if f['name'] not in seen:
            seen.add(f['name'])
            final_list.append(f)
    return final_list


def context_dependent_features(features):
    """Features whose value can change through a context override."""
    validate_runtime_features_graph(features)
    feature_map = {str(f['name']): f for f in features}
    dependent = set(str(f['name']) for f in overridable_features(features))

    def visit(name):
        if name in dependent:
            return True
        feature = feature_map[name]
        if any(visit(other) for other in
               feature['depends_on'] + feature['implied_by']):
            dependent.add(name)
            return True
        return False

    for name in feature_map:
        visit(name)
    return dependent
