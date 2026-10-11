// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunk_3d_order.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "cc/base/math_util.h"
#include "ui/gfx/geometry/quad_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace blink {
namespace {

// Match viz::DrawPolygon's tolerance for points on a splitting plane.
constexpr float kPlaneThickness = 0.05f;

struct Polygon {
  PaintChunk3DFragment fragment;
  gfx::Vector3dF normal;
};

void AppendVertex(Vector<gfx::Point3F>& vertices,
                  const gfx::Point3F& point) {
  if (vertices.empty() || vertices.back() != point) {
    vertices.push_back(point);
  }
}

// Classify or split a convex polygon, retaining its original plane normal.
// Recomputing the normal of a narrow fragment amplifies rounding error.
void SplitPolygon(const Polygon& splitter,
                  Polygon polygon,
                  Vector<Polygon>& front,
                  Vector<Polygon>& back,
                  Vector<Polygon>& coplanar) {
  const auto& vertices = polygon.fragment.vertices;
  Vector<float> distances;
  distances.ReserveInitialCapacity(vertices.size());
  bool has_front = false;
  bool has_back = false;
  for (const auto& vertex : vertices) {
    float distance = gfx::DotProduct(
        vertex - splitter.fragment.vertices.front(), splitter.normal);
    if (std::abs(distance) <= kPlaneThickness) {
      distance = 0;
    }
    has_front |= distance > 0;
    has_back |= distance < 0;
    distances.push_back(distance);
  }
  if (!has_front && !has_back) {
    coplanar.push_back(std::move(polygon));
    return;
  }
  if (!has_back) {
    front.push_back(std::move(polygon));
    return;
  }
  if (!has_front) {
    back.push_back(std::move(polygon));
    return;
  }

  Polygon front_part{{polygon.fragment.plane_index, {}, true}, polygon.normal};
  Polygon back_part{{polygon.fragment.plane_index, {}, true}, polygon.normal};
  for (wtf_size_t i = 0; i < vertices.size(); ++i) {
    wtf_size_t next = (i + 1) % vertices.size();
    if (distances[i] >= 0) {
      AppendVertex(front_part.fragment.vertices, vertices[i]);
    }
    if (distances[i] <= 0) {
      AppendVertex(back_part.fragment.vertices, vertices[i]);
    }
    if ((distances[i] < 0 && distances[next] > 0) ||
        (distances[i] > 0 && distances[next] < 0)) {
      double ratio = static_cast<double>(distances[i]) /
                     (distances[i] - distances[next]);
      const auto& a = vertices[i];
      const auto& b = vertices[next];
      gfx::Point3F intersection(a.x() + (b.x() - a.x()) * ratio,
                                a.y() + (b.y() - a.y()) * ratio,
                                a.z() + (b.z() - a.z()) * ratio);
      AppendVertex(front_part.fragment.vertices, intersection);
      AppendVertex(back_part.fragment.vertices, intersection);
    }
  }
  if (front_part.fragment.vertices.size() >= 3) {
    front.push_back(std::move(front_part));
  }
  if (back_part.fragment.vertices.size() >= 3) {
    back.push_back(std::move(back_part));
  }
}

struct OrderTask {
  Vector<Polygon> polygons;
  bool emit = false;
};

}  // namespace

Vector<PaintChunk3DFragment> OrderPaintChunk3DPlanes(
    base::span<const PaintChunk3DPlane> planes) {
  Vector<Polygon> polygons;
  polygons.ReserveInitialCapacity(base::checked_cast<wtf_size_t>(planes.size()));
  for (size_t index = 0; index < planes.size(); ++index) {
    std::array<gfx::Point3F, 6> vertices;
    int vertex_count = 0;
    cc::MathUtil::MapClippedQuad3d(planes[index].transform,
                                  gfx::QuadF(planes[index].bounds), vertices,
                                  &vertex_count);
    if (vertex_count < 3) {
      continue;
    }
    Polygon polygon{{index, {}, false}, {}};
    polygon.fragment.vertices.append_range(
        base::span(vertices).first(base::checked_cast<size_t>(vertex_count)));
    // As in DrawPolygon::ConstructNormal, average diagonal cross products.
    size_t delta = vertex_count / 2;
    for (size_t i = 1; i + delta < polygon.fragment.vertices.size(); ++i) {
      polygon.normal += gfx::CrossProduct(vertices[i] - vertices[0],
                                           vertices[i + delta] - vertices[0]);
    }
    float length = polygon.normal.Length();
    if (!length || !std::isfinite(length)) {
      continue;
    }
    polygon.normal.InvScale(length);
    polygons.push_back(std::move(polygon));
  }

  Vector<PaintChunk3DFragment> ordered;
  Vector<OrderTask> pending;
  pending.push_back(OrderTask{std::move(polygons)});
  // An explicit work stack avoids recursion proportional to page size for a
  // long preserve-3d list. This is the same far/plane/near traversal as BspTree.
  while (!pending.empty()) {
    OrderTask task = std::move(pending.back());
    pending.pop_back();
    if (task.emit) {
      for (auto& polygon : task.polygons) {
        ordered.push_back(std::move(polygon.fragment));
      }
      continue;
    }
    if (task.polygons.empty()) {
      continue;
    }
    Polygon splitter = std::move(task.polygons.front());
    Vector<Polygon> front;
    Vector<Polygon> back;
    Vector<Polygon> coplanar;
    for (wtf_size_t i = 1; i < task.polygons.size(); ++i) {
      SplitPolygon(splitter, std::move(task.polygons[i]), front, back, coplanar);
    }
    bool camera_in_front = splitter.normal.z() > 0;
    coplanar.push_back(std::move(splitter));
    std::stable_sort(coplanar.begin(), coplanar.end(),
                     [](const Polygon& a, const Polygon& b) {
                       return a.fragment.plane_index < b.fragment.plane_index;
                     });
    pending.push_back(
        OrderTask{camera_in_front ? std::move(front) : std::move(back)});
    pending.push_back(OrderTask{std::move(coplanar), true});
    pending.push_back(
        OrderTask{camera_in_front ? std::move(back) : std::move(front)});
  }
  return ordered;
}

}  // namespace blink
