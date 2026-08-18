// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides some helper methods for building and rendering an
// internal html page.
// Note: This file is named with the 'jstemplate_' prefix purely for historical
// reasons, as it no longer uses jstemplate (see crbug.com/378692755 for
// context).

#ifndef UI_BASE_WEBUI_JSTEMPLATE_BUILDER_H_
#define UI_BASE_WEBUI_JSTEMPLATE_BUILDER_H_

#include <string>
#include <string_view>

#include "base/component_export.h"
#include "base/values.h"

namespace webui {

// A helper function that generates a string of HTML to be loaded.  The
// string includes
// GetI18nTemplateHtml() was declared here. It returned the HTML template with
// ui::TemplateReplacements populated plus a <script> tag carrying
// `loadTimeData`. It had no callers and its resource is gone with
// ui/webui/resources.

// Assigns the given json data into |loadTimeData|, without a <script> tag.
COMPONENT_EXPORT(UI_BASE)
void AppendJsonJS(const base::DictValue& json,
                  std::string* output,
                  bool from_js_module);

}  // namespace webui

#endif  // UI_BASE_WEBUI_JSTEMPLATE_BUILDER_H_
