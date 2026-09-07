# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json5_generator
import os
import template_expander
from collections import defaultdict
from make_runtime_features_utilities import context_dependent_features


class PermissionsPolicyFeatureWriter(json5_generator.Writer):
    file_basename = 'policy_helper'

    def __init__(self, json5_file_path, output_dir):
        super(PermissionsPolicyFeatureWriter,
              self).__init__(json5_file_path, output_dir)

        for path in json5_file_path:
            file_root, file_ext = os.path.splitext(path)
            override_file_path = f"{file_root}.override{file_ext}"
            if os.path.exists(override_file_path):
                self.json5_file.load_override_file(override_file_path)

        runtime_features = []
        permissions_policy_features = []
        # Note: there can be feature with same 'name' attribute in
        # document_policy_features and in permissions_policy_features.
        # They are supposed to have the same 'depends_on' attribute.
        # However, their permissions_policy_name and document_policy_name
        # might be different.
        document_policy_features = []

        def to_devtools_enum_format(permissions_policy_name):
            """ Convert '-' separated permissions_policy_name to cammel case devtool enum name """
            return ''.join([
                name.capitalize()
                for name in permissions_policy_name.split('-')
            ])

        name_to_permissions_policy_map = {}
        for feature in self.json5_file.name_dictionaries:
            if feature['permissions_policy_name']:
                feature['devtools_enum_name'] = to_devtools_enum_format(
                    feature['permissions_policy_name'])
                permissions_policy_features.append(feature)
                name_to_permissions_policy_map[feature['name']] = feature
            elif feature['document_policy_name']:
                document_policy_features.append(feature)
            else:
                runtime_features.append(feature)

        context_dependent_set = context_dependent_features(runtime_features)
        pp_context_dependency_map = defaultdict(list)
        dp_context_dependency_map = defaultdict(list)
        pp_runtime_dependency_map = {}
        dp_runtime_dependency_map = {}
        runtime_to_permissions_policy_map = defaultdict(list)
        runtime_to_document_policy_map = defaultdict(list)
        for feature in permissions_policy_features + document_policy_features:
            if feature['depends_on']:
                dependency_map = (pp_runtime_dependency_map if
                                  feature['permissions_policy_name'] else
                                  dp_runtime_dependency_map)
                dependency_map[feature['name']] = feature['depends_on']
            for dependency in feature['depends_on']:
                if str(dependency) in context_dependent_set:
                    if feature['permissions_policy_name']:
                        pp_context_dependency_map[feature['name']].append(
                            dependency)
                    else:
                        dp_context_dependency_map[feature['name']].append(
                            dependency)
                else:
                    if feature['permissions_policy_name']:
                        runtime_to_permissions_policy_map[dependency].append(
                            feature['name'])
                    else:
                        runtime_to_document_policy_map[dependency].append(
                            feature['name'])

        self._outputs = {
            self.file_basename + '.cc':
            template_expander.use_jinja(
                'templates/' + self.file_basename + '.cc.tmpl')(lambda: {
                    'header_guard':
                    self.make_header_guard(self._relative_output_dir + self.
                                           file_basename + '.h'),
                    'input_files':
                    self._input_files,
                    'permissions_policy_features':
                    permissions_policy_features,
                    'name_to_permissions_policy_map':
                    name_to_permissions_policy_map,
                    'document_policy_features':
                    document_policy_features,
                    'pp_runtime_dependency_map': pp_runtime_dependency_map,
                    'dp_runtime_dependency_map': dp_runtime_dependency_map,
                    'pp_context_dependency_map':
                    pp_context_dependency_map,
                    'dp_context_dependency_map':
                    dp_context_dependency_map,
                    'runtime_to_permissions_policy_map':
                    runtime_to_permissions_policy_map,
                    'runtime_to_document_policy_map':
                    runtime_to_document_policy_map
                }),
        }


if __name__ == '__main__':
    json5_generator.Maker(PermissionsPolicyFeatureWriter).main()
