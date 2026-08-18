// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/webui/jstemplate_builder.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"

namespace webui {

// AppendJsonHtml() lived here, wrapping AppendJsonJS()'s output in a <script>
// tag for GetI18nTemplateHtml(). It went with its only caller.

// GetI18nTemplateHtml() and its AppendLoadTimeData() helper lived here. They
// inlined IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS, from the deleted
// ui/webui/resources tree, into a chrome:// page as a <script> tag. Nothing in
// this tree called GetI18nTemplateHtml(), and there is no script engine to run
// what it injected.

void AppendJsonJS(const base::DictValue& json,
                  std::string* output,
                  bool from_js_module) {
  if (from_js_module) {
    // If the script is being imported as a module, import |loadTimeData| in
    // order to allow assigning the localized strings to loadTimeData.data.
    output->append("import {loadTimeData} from ");
    output->append("'//resources/js/load_time_data.js';\n");

#if BUILDFLAG(IS_CHROMEOS)
    // Imported for the side effect of setting the |window.loadTimeData| global,
    // which is relied on by ChromeOS Ash Tast Tests and some browser tests.
    // See https://www.crbug.com/1395148.
    output->append("import '//resources/ash/common/load_time_data.m.js';\n");
#endif  // BUILDFLAG(IS_CHROMEOS)
  }

  std::optional<std::string> jstext = base::WriteJson(json);
  CHECK(jstext);
  output->append("loadTimeData.data = ");
  output->append(*jstext);
  output->append(";");
}

}  // namespace webui
