// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/switches.h"

namespace blink {
namespace switches {

// Allows processing of input before a frame has been committed.
// TODO(crbug.com/987626): Used by headless. Look for a way not
// involving a command line switch.
const char kAllowPreCommitInput[] = "allow-pre-commit-input";

// Set blink settings. Format is <name>[=<value],<name>[=<value>],...
// The names are declared in Settings.json5. For boolean type, use "true",
// "false", or omit '=<value>' part to set to true. For enum type, use the int
// value of the enum value. Applied after other command line flags and prefs.
const char kBlinkSettings[] = "blink-settings";

// Sets dark mode settings. Format is [<param>=<value>],[<param>=<value>],...
// The params take either int or float values. If params are not specified,
// the default dark mode settings is used. Valid params are given below.
// "InversionAlgorithm" takes int value of DarkModeInversionAlgorithm enum.
// "ImagePolicy" takes int value of DarkModeImagePolicy enum.
// "ForegroundBrightnessThreshold" takes 0 to 255 int value.
// "BackgroundBrightnessThreshold" takes 0 to 255 int value.
// "ContrastPercent" takes -1.0 to 1.0 float value. Higher the value, more
// the contrast.
const char kDarkModeSettings[] = "dark-mode-settings";

// Overrides data: URLs in SVGUseElement deprecation through enterprise policy.
const char kDataUrlInSvgUseEnabled[] = "data-url-in-svg-use-enabled";

// Toggles partitioning of Blob URLs through enterprise policy.
const char kDisableBlobUrlPartitioning[] = "disable-blob-url-partitioning";

// Sets the tile size used by composited layers.
const char kDefaultTileWidth[] = "default-tile-width";
const char kDefaultTileHeight[] = "default-tile-height";

// Disallow image animations to be reset to the beginning to avoid skipping
// many frames. Only effective if compositor image animations are enabled.
const char kDisableImageAnimationResync[] = "disable-image-animation-resync";

// Logs Runtime Call Stats. --single-process also needs to be used along with
// this for the stats to be logged.
const char kDumpRuntimeCallStats[] = "dump-blink-runtime-call-stats";

// Enable desktop Android scrollbars.
const char kEnableDesktopAndroidScrollbars[] =
    "enable-desktop-android-scrollbars";

// Enables raster side dark mode for images.
const char kEnableRasterSideDarkModeForImages[] =
    "enable-raster-side-dark-mode-for-images";

#if BUILDFLAG(IS_CHROMEOS)
// Enables placing WebGL textures into overlays and giving them low-latency
// usages. Restricted to ChromeOS, where this functionality is enabled on a
// per-board basis. NOTE: The value for the switch name should not be renamed
// for historical reasons (i.e., doing so would require updating this as well
// and coordinating rollout:
// https://source.chromium.org/chromiumos/chromiumos/codesearch/+/main:src/platform2/login_manager/chromium_command_builder.cc;drc=ed9561e7ab8a71fe4a9d9946cea99df8ebfc9449;l=558).
const char kEnableOverlaysAndLowLatencyUsageForWebGL[] =
    "enable-webgl-image-chromium";
#endif

// Used to communicate managed policy for the IntensiveWakeUpThrottling feature.
// This feature is typically controlled by base::Feature (see
// renderer/platform/scheduler/common/features.*) but requires an enterprise
// policy override. This is implicitly a tri-state, and can be either unset, or
// set to "1" for force enable, or "0" for force disable.
extern const char kIntensiveWakeUpThrottlingPolicy[] =
    "intensive-wake-up-throttling-policy";
extern const char kIntensiveWakeUpThrottlingPolicy_ForceDisable[] = "0";
extern const char kIntensiveWakeUpThrottlingPolicy_ForceEnable[] = "1";

// A command line to indicate if there ia any legacy tech report urls being set.
// If so, we will send report from blink to browser process.
extern const char kLegacyTechReportPolicyEnabled[] =
    "legacy-tech-report-policy-enabled";

// Sets the width and height above which a composited layer will get tiled.
const char kMaxUntiledLayerHeight[] = "max-untiled-layer-height";
const char kMaxUntiledLayerWidth[] = "max-untiled-layer-width";

// Used to communicate managed policy for CSSCustomStateDeprecatedSyntax. This
// feature is typically controlled by a RuntimeEnabledFeature, but requires an
// enterprise policy override.
extern const char kCSSCustomStateDeprecatedSyntaxEnabled[] =
    "css-custom-state-deprecated-syntax-enabled";

// Sets the timeout seconds of the network-quiet timers in IdlenessDetector.
// Used by embedders who want to change the timeout time in order to run web
// contents on various embedded devices and changeable network bandwidths in
// different regions. For example, it's useful when using FirstMeaningfulPaint
// signal to dismiss a splash screen.
const char kNetworkQuietTimeout[] = "network-quiet-timeout";

// Visibly render a border around paint rects in the web page to help debug
// and study painting behavior.  Unlike "show-paint-rects", this only visualizes
// new contentful paints for each element (e.g. for LCP).
const char kShowContentfulPaintRects[] = "show-contentful-paint-rects";

// Visibly render a border around layout shift rects in the web page to help
// debug and study layout shifts.
const char kShowLayoutShiftRegions[] = "show-layout-shift-regions";

// Visibly render a border around paint rects in the web page to help debug
// and study painting behavior.
const char kShowPaintRects[] = "show-paint-rects";

// Controls how text selection granularity changes when touch text selection
// handles are dragged. Should be "character" or "direction". If not specified,
// the platform default is used.
const char kTouchTextSelectionStrategy[] = "touch-selection-strategy";
const char kTouchTextSelectionStrategy_Character[] = "character";
const char kTouchTextSelectionStrategy_Direction[] = "direction";

// Override mechanism for preserving the old non-standard behavior of CSS zoom.
const char kDisableStandardizedBrowserZoom[] =
    "disable-standardized-browser-zoom";

// Specifies the flags passed to JS engine.
const char kJavaScriptFlags[] = "js-flags";

// Used to communicate managed policy for WebAudioBypassOutputBuffering.  This
// feature is typically controlled by a RuntimeEnabledFeature, but requires an
// enterprise policy override.
const char kWebAudioBypassOutputBufferingOptOut[] =
    "web-audio-bypass-output-buffering-opt-out";

const char kDisableBackForwardCacheForWebSockets[] =
    "disable-back-forward-cache-for-web-sockets";

// Override mechanism for ReduceAcceptLanguage. This feature is typically
// controlled by base features, but requires an enterprise policy override.
const char kDisableReduceAcceptLanguage[] = "disable-reduce-accept-language";

// A switch that controls XSLT availability via enterprise policy.
const char kXSLTEnabledPolicy[] = "xslt-enabled-policy";

// A switch that controls RestrictBackgroundFetchFromServiceWorker availability
// via enterprise policy.
const char kRestrictBackgroundFetchFromServiceWorker[] =
    "restrict-background-fetch-from-service-worker";

// A switch passed to renderer processes hosting Top Chrome WebUI.
const char kTopChromeWebUI[] = "top-chrome-webui";

}  // namespace switches
}  // namespace blink
