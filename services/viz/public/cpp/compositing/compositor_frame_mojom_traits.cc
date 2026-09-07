// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/public/cpp/compositing/compositor_frame_mojom_traits.h"

namespace mojo {

bool RenderPassExists(viz::CompositorRenderPassId pass_id,
                      const viz::CompositorRenderPassList& render_passes) {
  for (const auto& pass : render_passes) {
    if (pass->id == pass_id)
      return true;
  }

  return false;
}

// static
base::expected<void, DeserializationError>
StructTraits<viz::mojom::CompositorFrameDataView, viz::CompositorFrame>::Read(
    viz::mojom::CompositorFrameDataView data,
    viz::CompositorFrame* out) {
  if (!data.ReadPasses(&out->render_pass_list)) {
    return base::unexpected(DeserializationError());
  }

  if (out->render_pass_list.empty()) {
    return base::unexpected(DeserializationError());
  }

  if (out->render_pass_list.back()->output_rect.size().IsEmpty()) {
    return base::unexpected(DeserializationError());
  }

  if (!data.ReadMetadata(&out->metadata)) {
    return base::unexpected(DeserializationError());
  }

  // Ensure that all render passes referenced by shared elements are present in
  // the CompositorFrame.
  for (size_t i = 0; i < out->metadata.transition_directives.size(); ++i) {
    const auto& directive = out->metadata.transition_directives[i];
    if (directive.type() !=
        viz::CompositorFrameTransitionDirective::Type::kSave) {
      DCHECK(directive.shared_elements().empty());
      continue;
    }

    for (const auto& shared_element : directive.shared_elements()) {
      if (shared_element.render_pass_id.is_null())
        continue;

      if (!RenderPassExists(shared_element.render_pass_id,
                            out->render_pass_list)) {
        return base::unexpected(DeserializationError::CustomCode(i));
      }
    }
  }

  if (!data.ReadResources(&out->resource_list)) {
    return base::unexpected(DeserializationError());
  }

  return base::ok();
}

}  // namespace mojo
