// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_PROFILE_H_
#define SHOT_SHOT_PROFILE_H_

#include <cstdlib>
#include <string>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

namespace shot {

// Whether SHOT_PROFILE is set. The renderer's per-round timings, the memory
// stages and the runtime's start-up split all answer to this one switch; they
// print through LOG(INFO), so --verbose (or SHOT_VERBOSE=1 through the
// library) is needed as well.
//
// Read once: the environment does not change while the process runs, and a
// resident worker asks thousands of times.
inline bool ProfileEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("SHOT_PROFILE");
    return value && *value && *value != '0';
  }();
  return enabled;
}

// A stopwatch for a sequence of named steps, printed as one line at the end
// so that a start-up split reads as a whole rather than as a dozen
// interleaved lines. Does nothing at all unless profiling is on.
class ProfileStages {
 public:
  explicit ProfileStages(const char* what)
      : enabled_(ProfileEnabled()), what_(what) {
    if (enabled_) {
      started_ = base::TimeTicks::Now();
      last_ = started_;
    }
  }
  ProfileStages(const ProfileStages&) = delete;
  ProfileStages& operator=(const ProfileStages&) = delete;
  ~ProfileStages() { Finish(); }

  // Prints the line now, for a scope that ends in an exit rather than a
  // return. Prints once.
  void Finish() {
    if (!enabled_) {
      return;
    }
    enabled_ = false;
    LOG(INFO) << "shot: profile " << what_ << line_ << " total="
              << (base::TimeTicks::Now() - started_).InMillisecondsF();
  }

  // Closes the step that has been running since the previous Mark() (or the
  // construction) and names it.
  void Mark(const char* step) {
    if (!enabled_) {
      return;
    }
    const base::TimeTicks now = base::TimeTicks::Now();
    line_ += " ";
    line_ += step;
    line_ += "=";
    line_ += base::NumberToString((now - last_).InMillisecondsF());
    last_ = now;
  }

 private:
  bool enabled_;
  const char* what_;
  base::TimeTicks started_;
  base::TimeTicks last_;
  std::string line_;
};

}  // namespace shot

#endif  // SHOT_SHOT_PROFILE_H_
