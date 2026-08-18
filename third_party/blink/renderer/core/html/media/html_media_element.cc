/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/media/html_media_element.h"

#include <algorithm>
#include <limits>

#include "base/feature_list.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_can_play_type_result.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/attribute.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html/html_source_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_element_controls_list.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/html/media/media_error.h"
#include "third_party/blink/renderer/core/html/time_ranges.h"
#include "third_party/blink/renderer/core/html/track/audio_track.h"
#include "third_party/blink/renderer/core/html/track/audio_track_list.h"
#include "third_party/blink/renderer/core/html/track/automatic_track_selection.h"
#include "third_party/blink/renderer/core/html/track/cue_timeline.h"
#include "third_party/blink/renderer/core/html/track/html_track_element.h"
#include "third_party/blink/renderer/core/html/track/loadable_text_track.h"
#include "third_party/blink/renderer/core/html/track/text_track_container.h"
#include "third_party/blink/renderer/core/html/track/text_track_list.h"
#include "third_party/blink/renderer/core/html/track/video_track.h"
#include "third_party/blink/renderer/core/html/track/video_track_list.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/keywords.h"
#include "third_party/blink/renderer/core/layout/layout_media.h"
#include "third_party/blink/renderer/platform/bindings/exception_messages.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/disallow_new_wrapper.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/network/mime/content_type.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cancellable_task.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

using WeakMediaElementSet = GCedHeapHashSet<WeakMember<HTMLMediaElement>>;
using DocumentElementSetMap =
    HeapHashMap<WeakMember<Document>, Member<WeakMediaElementSet>>;

namespace {

DocumentElementSetMap& DocumentToElementSetMap() {
  using DocumentElementSetMapHolder = DisallowNewWrapper<DocumentElementSetMap>;
  DEFINE_STATIC_LOCAL(Persistent<DocumentElementSetMapHolder>, holder,
                      (MakeGarbageCollected<DocumentElementSetMapHolder>()));
  return holder->Value();
}

void AddElementToDocumentMap(HTMLMediaElement* element, Document* document) {
  DocumentElementSetMap& map = DocumentToElementSetMap();
  WeakMediaElementSet* set = nullptr;
  auto it = map.find(document);
  if (it == map.end()) {
    set = MakeGarbageCollected<WeakMediaElementSet>();
    map.insert(document, set);
  } else {
    set = it->value;
  }
  set->insert(element);
}

void RemoveElementFromDocumentMap(HTMLMediaElement* element,
                                  Document* document) {
  DocumentElementSetMap& map = DocumentToElementSetMap();
  auto it = map.find(document);
  CHECK(it != map.end());
  WeakMediaElementSet* set = it->value;
  set->erase(element);
  if (set->empty())
    map.erase(it);
}

}  // anonymous namespace

// static
MIMETypeRegistry::SupportsType HTMLMediaElement::GetSupportsType(
    const ContentType& content_type) {
  String type = content_type.GetType().DeprecatedLower();
  // The codecs string is not lower-cased because MP4 values are case sensitive
  // per http://tools.ietf.org/html/rfc4281#page-7.
  String type_codecs = content_type.Parameter("codecs");

  if (type.empty())
    return MIMETypeRegistry::kNotSupported;

  // 4.8.12.3 MIME types - The canPlayType(type) method must return the empty
  // string if type is a type that the user agent knows it cannot render or is
  // the type "application/octet-stream"
  if (type == "application/octet-stream")
    return MIMETypeRegistry::kNotSupported;

  // |contentType| could be handled using ParsedContentType, but there are
  // still a lot of sites using codec strings that don't work with the
  // stricter parsing rules.
  return MIMETypeRegistry::SupportsMediaMIMEType(type, type_codecs);
}

// static
void HTMLMediaElement::OnMediaControlsEnabledChange(Document*) {
  // MediaControls no longer exists (there is no player for it to control),
  // so there is nothing left to notify.
}

bool HTMLMediaElement::IsHLSURL(const KURL& url) {
  // Keep the same logic as in media_codec_util.h.
  if (url.IsNull() || url.IsEmpty())
    return false;

  if (!url.IsLocalFile() && !url.ProtocolIsInHttpFamily()) {
    return false;
  }

  return url.GetPath().ends_with(".m3u8");
}

HTMLMediaElement::HTMLMediaElement(const QualifiedName& tag_name,
                                   Document& document)
    : HTMLElement(tag_name, document),
      ActiveScriptWrappable<HTMLMediaElement>({}),
      ExecutionContextLifecycleStateObserver(GetExecutionContext()),
      async_event_queue_(
          MakeGarbageCollected<EventQueue>(GetExecutionContext(),
                                           TaskType::kMediaElementEvent)),
      audio_tracks_(MakeGarbageCollected<AudioTrackList>(*this)),
      video_tracks_(MakeGarbageCollected<VideoTrackList>(*this)),
      controls_list_(MakeGarbageCollected<HTMLMediaElementControlsList>(this)) {
  DVLOG(1) << "HTMLMediaElement(" << static_cast<void*>(this) << ")";

  SetHasCustomStyleCallbacks();
  AddElementToDocumentMap(this, &document);

  UseCounter::Count(document, WebFeature::kHTMLMediaElement);
}

HTMLMediaElement::~HTMLMediaElement() {
  DVLOG(1) << "~HTMLMediaElement(" << static_cast<void*>(this) << ")";
}

void HTMLMediaElement::Dispose() {}

void HTMLMediaElement::DidMoveToNewDocument(Document& old_document) {
  DVLOG(3) << "didMoveToNewDocument(" << static_cast<void*>(this) << ")";

  if (cue_timeline_) {
    cue_timeline_->DidMoveToNewDocument(old_document);
  }

  RemoveElementFromDocumentMap(this, &old_document);
  AddElementToDocumentMap(this, &GetDocument());
  SetExecutionContext(GetExecutionContext());

  HTMLElement::DidMoveToNewDocument(old_document);
}

FocusableState HTMLMediaElement::SupportsFocus(
    UpdateBehavior update_behavior) const {
  // TODO(https://crbug.com/911882): Depending on result of discussion, remove.
  if (ownerDocument()->IsMediaDocument()) {
    return FocusableState::kNotFocusable;
  }

  // If no controls specified, we should still be able to focus the element if
  // it has tabIndex.
  if (ShouldShowControls()) {
    return FocusableState::kFocusable;
  }
  return HTMLElement::SupportsFocus(update_behavior);
}

FocusableState HTMLMediaElement::IsFocusableState(
    UpdateBehavior update_behavior) const {
  if (!IsFullscreen()) {
    return SupportsFocus(update_behavior);
  }
  return HTMLElement::IsFocusableState(update_behavior);
}

int HTMLMediaElement::DefaultTabIndex() const {
  return 0;
}

void HTMLMediaElement::ParseAttribute(
    const AttributeModificationParams& params) {
  const QualifiedName& name = params.name;
  if (name == html_names::kSrcAttr) {
    // A change to the src attribute can affect intrinsic size (the poster is
    // shown until there is a resource, and there never is one), which in turn
    // requires a style recalc.
    SetNeedsStyleRecalc(kLocalStyleChange,
                        StyleChangeReasonForTracing::FromAttribute(name));
  } else if (name == html_names::kControlslistAttr) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kHTMLMediaElementControlsListAttribute);
    if (params.old_value != params.new_value) {
      controls_list_->DidUpdateAttributeValue(params.old_value,
                                              params.new_value);
    }
  } else if (name == html_names::kControlsAttr) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kHTMLMediaElementControlsAttribute);
  } else if (name == html_names::kMutedAttr) {
    if (!RuntimeEnabledFeatures::MediaElementMutedDefaultStateEnabled() &&
        params.reason == AttributeModificationReason::kByParser) {
      muted_ = true;
    }
  } else {
    HTMLElement::ParseAttribute(params);
  }
}

// This method is being used as a way to know that cloneNode finished cloning
// attribute as there is no callback notifying about the end of a cloning
// operation. Indeed, it is required per spec to set the muted state based on
// the content attribute when the object is created.
void HTMLMediaElement::CloneNonAttributePropertiesFrom(const Element& other,
                                                       NodeCloningData& data) {
  HTMLElement::CloneNonAttributePropertiesFrom(other, data);

  if (!RuntimeEnabledFeatures::MediaElementMutedDefaultStateEnabled() &&
      FastHasAttribute(html_names::kMutedAttr)) {
    muted_ = true;
  }
}

void HTMLMediaElement::FinishParsingChildren() {
  HTMLElement::FinishParsingChildren();

  // When the blocked-on-parser flag is cleared, honor user preferences for
  // automatic text track selection and populate the list of pending text
  // tracks.
  HonorUserPreferencesForAutomaticTextTrackSelection();
  AddPendingTextTracksFromCurrentList();
}

bool HTMLMediaElement::LayoutObjectIsNeeded(const DisplayStyle& style) const {
  return ShouldShowControls() && HTMLElement::LayoutObjectIsNeeded(style);
}

LayoutObject* HTMLMediaElement::CreateLayoutObject(const ComputedStyle&) {
  return MakeGarbageCollected<LayoutMedia>(this);
}

Node::InsertionNotificationRequest HTMLMediaElement::InsertedInto(
    ContainerNode& insertion_point) {
  DVLOG(3) << "insertedInto(" << static_cast<void*>(this) << ")";

  HTMLElement::InsertedInto(insertion_point);
  if (insertion_point.isConnected()) {
    UseCounter::Count(GetDocument(), WebFeature::kHTMLMediaElementInDocument);
  }

  return kInsertionDone;
}

void HTMLMediaElement::RemovedFrom(ContainerNode& insertion_point) {
  DVLOG(3) << "removedFrom(" << static_cast<void*>(this) << ")";
  HTMLElement::RemovedFrom(insertion_point);
}

void HTMLMediaElement::AttachLayoutTree(AttachContext& context) {
  HTMLElement::AttachLayoutTree(context);

  UpdateLayoutObject();
}

void HTMLMediaElement::DidRecalcStyle(const StyleRecalcChange change) {
  if (!change.ReattachLayoutTree())
    UpdateLayoutObject();
}

void HTMLMediaElement::ScheduleAutomaticTextTrackSelection() {
  DVLOG(3) << "scheduleAutomaticTextTrackSelection(" << static_cast<void*>(this)
           << ")";

  // Queue "honor user preferences for automatic text track selection" on the
  // media element event task source so it runs after media events already
  // queued there; the spec requires this selection to be a task that does not
  // preempt pending media events. Scheduling at most one task at a time
  // coalesces several track additions in the same turn into a single
  // selection pass.
  if (!text_track_selection_task_handle_.IsActive()) {
    text_track_selection_task_handle_ = PostCancellableTask(
        *GetDocument().GetTaskRunner(TaskType::kMediaElementEvent), FROM_HERE,
        BindOnce(&HTMLMediaElement::
                     HonorUserPreferencesForAutomaticTextTrackSelection,
                 WrapWeakPersistent(this)));
  }
}

void HTMLMediaElement::ScheduleEvent(Event* event) {
  async_event_queue_->EnqueueEvent(FROM_HERE, *event);
}

void HTMLMediaElement::ScheduleNamedEvent(const AtomicString& event_name) {
  Event* event = Event::CreateCancelable(event_name);
  event->SetTarget(this);
  ScheduleEvent(event);
}

MediaError* HTMLMediaElement::error() const {
  return error_.Get();
}

void HTMLMediaElement::SetSrc(const AtomicString& url) {
  setAttribute(html_names::kSrcAttr, url);
}

String HTMLMediaElement::preload() const {
  const AtomicString& preload = FastGetAttribute(html_names::kPreloadAttr);
  if (EqualIgnoringAsciiCase(preload, keywords::kNone))
    return keywords::kNone;
  if (EqualIgnoringAsciiCase(preload, "metadata"))
    return "metadata";
  // "auto" is the default.
  return keywords::kAuto;
}

void HTMLMediaElement::setPreload(const AtomicString& preload) {
  setAttribute(html_names::kPreloadAttr, preload);
}

bool HTMLMediaElement::HasMediaSources() const {
  return FastHasAttribute(html_names::kSrcAttr) ||
         Traversal<HTMLSourceElement>::FirstChild(*this);
}

TimeRanges* HTMLMediaElement::buffered() const {
  // No player ever exists to report buffered ranges.
  return MakeGarbageCollected<TimeRanges>();
}

TimeRanges* HTMLMediaElement::played() {
  // Nothing is ever played, so the played ranges are always empty.
  return MakeGarbageCollected<TimeRanges>();
}

TimeRanges* HTMLMediaElement::seekable() const {
  // No player ever exists to report seekable ranges.
  return MakeGarbageCollected<TimeRanges>();
}

void HTMLMediaElement::load() {
  DVLOG(1) << "load(" << static_cast<void*>(this) << ")";
  // The full HTML "media element load algorithm" exists to drive a
  // WebMediaPlayer through network fetch, demux and decode: pick a <source>
  // or the src attribute, ask the player to open it, and step networkState /
  // readyState through NETWORK_LOADING / HAVE_METADATA / etc. as the player
  // reports progress. This build has no WebMediaPlayer (the entire //media
  // pipeline was cut, see the file comment in the header), so there is
  // nothing for load() to select or open: no fetch happens, no track is ever
  // populated, and the element never leaves NETWORK_EMPTY / HAVE_NOTHING.
  // <video poster> -- the only pixels a <video> can show here -- is loaded
  // independently of this by HTMLVideoElement's own HTMLImageLoader.
}

V8CanPlayTypeResult HTMLMediaElement::canPlayType(
    const String& mime_type) const {
  MIMETypeRegistry::SupportsType support =
      GetSupportsType(ContentType(mime_type));

  V8CanPlayTypeResult can_play =
      V8CanPlayTypeResult(V8CanPlayTypeResult::Enum::k);

  // 4.8.12.3
  switch (support) {
    case MIMETypeRegistry::kNotSupported:
      break;
    case MIMETypeRegistry::kMaybeSupported:
      can_play = V8CanPlayTypeResult(V8CanPlayTypeResult::Enum::kMaybe);
      break;
    case MIMETypeRegistry::kSupported:
      can_play = V8CanPlayTypeResult(V8CanPlayTypeResult::Enum::kProbably);
      break;
  }

  return can_play;
}

bool HTMLMediaElement::TextTracksAreReady() const {
  // https://html.spec.whatwg.org/#text-track-readiness-state
  // The text tracks of a media element are ready when both the element's list
  // of pending text tracks is empty and the element's blocked-on-parser flag
  // is false.
  for (const auto& text_track : text_tracks_when_resource_selection_began_) {
    if (text_track->GetReadinessState() == TextTrack::kLoading ||
        text_track->GetReadinessState() == TextTrack::kNotLoaded)
      return false;
  }

  return IsFinishedParsingChildren();
}

void HTMLMediaElement::TextTrackReadyStateChanged(TextTrack*) {
  // Used to also re-derive readyState from the (now nonexistent)
  // WebMediaPlayer and to notify MediaControls (also gone) about a track
  // that failed to load, so the CC button could be hidden. Neither
  // consumer exists any more.
}

void HTMLMediaElement::TextTrackModeChanged(TextTrack* track) {
  // Mark this track as "configured" so configureTextTracks won't change the
  // mode again.
  if (IsA<LoadableTextTrack>(track))
    track->SetHasBeenConfigured(true);

  if (track->IsRendered()) {
    GetDocument().GetStyleEngine().AddTextTrack(track);
  } else {
    GetDocument().GetStyleEngine().RemoveTextTrack(track);
  }

  ConfigureTextTrackDisplay();

  DCHECK(textTracks()->Contains(track));
  textTracks()->ScheduleChangeEvent();
}

void HTMLMediaElement::DisableAutomaticTextTrackSelection() {
  should_perform_automatic_track_selection_ = false;
}

bool HTMLMediaElement::SupportsSave() const {
  // There is no resource-selection algorithm to choose a URL and no player to
  // download from, so there is never anything to save.
  return false;
}

bool HTMLMediaElement::SupportsLoop() const {
  // Infinite streams don't have a clear end at which to loop. Nothing this
  // build can ever load is infinite, but keep the (harmless) spec check.
  return duration() != std::numeric_limits<double>::infinity();
}

// getReadyState(), defaultPlaybackRate() and playbackRate() are now trivial
// accessors of state that never changes on its own (no player advances any
// of it), so the header inlines them directly; the out-of-line definitions
// that used to live here were removed as duplicates (ODR redefinitions).

void HTMLMediaElement::setCurrentTime(double time) {
  current_time_ = time;
}

void HTMLMediaElement::setDefaultPlaybackRate(double rate) {
  if (default_playback_rate_ != rate) {
    default_playback_rate_ = rate;
    ScheduleNamedEvent(event_type_names::kRatechange);
  }
}

void HTMLMediaElement::setPlaybackRate(double rate,
                                       ExceptionState& exception_state) {
  DVLOG(3) << "setPlaybackRate(" << static_cast<void*>(this) << ", " << rate
           << ")";

  if (rate != 0.0 &&
      (rate < kMinPlaybackRate || rate > kMaxPlaybackRate)) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kNotSupportedError,
        ExceptionMessages::IndexOutsideRange(
            "playbackRate", rate, kMinPlaybackRate,
            ExceptionMessages::kInclusiveBound, kMaxPlaybackRate,
            ExceptionMessages::kInclusiveBound));
    return;
  }

  if (playback_rate_ != rate) {
    playback_rate_ = rate;
    ScheduleNamedEvent(event_type_names::kRatechange);
  }
}

bool HTMLMediaElement::ended() const {
  // There is never any playback to end.
  return false;
}

bool HTMLMediaElement::Autoplay() const {
  return FastHasAttribute(html_names::kAutoplayAttr);
}

std::optional<DOMExceptionCode> HTMLMediaElement::Play() {
  DVLOG(2) << "play(" << static_cast<void*>(this) << ")";
  // Every src this build could ever be given is unsupported: the entire
  // decode pipeline was cut, so GetSupportsType() (see canPlayType()) can
  // never return anything but kNotSupported. NotSupportedError is exactly
  // what a real browser returns from play() when the source can't be
  // decoded, so this is the honest answer, not a placeholder one.
  return DOMExceptionCode::kNotSupportedError;
}

void HTMLMediaElement::pause() {
  DVLOG(2) << "pause(" << static_cast<void*>(this) << ")";
  // paused_ starts, and stays, true: Play() never clears it. This is here so
  // that a script calling pause() on an already-paused element does not
  // observe anything strange (no-op, matches the spec for an already-paused
  // element).
}

void HTMLMediaElement::setPreservesPitch(bool preserves_pitch) {
  UseCounter::Count(GetDocument(), WebFeature::kPreservesPitch);
  preserves_pitch_ = preserves_pitch;
}

double HTMLMediaElement::latencyHint() const {
  // Parse error will fallback to std::numeric_limits<double>::quiet_NaN()
  double seconds = GetFloatingPointAttribute(html_names::kLatencyhintAttr);

  // Return NaN for invalid values.
  if (!std::isfinite(seconds) || seconds < 0)
    return std::numeric_limits<double>::quiet_NaN();

  return seconds;
}

void HTMLMediaElement::setLatencyHint(double seconds) {
  SetFloatingPointAttribute(html_names::kLatencyhintAttr, seconds);
}

bool HTMLMediaElement::Loop() const {
  return FastHasAttribute(html_names::kLoopAttr);
}

void HTMLMediaElement::SetLoop(bool b) {
  SetBooleanAttribute(html_names::kLoopAttr, b);
}

bool HTMLMediaElement::ShouldShowControls() const {
  // If the document is not active, then we should not show controls.
  if (!GetDocument().IsActive()) {
    return false;
  }

  Settings* settings = GetDocument().GetSettings();
  if (settings && !settings->GetMediaControlsEnabled()) {
    return false;
  }

  // If the user has explicitly shown or hidden the controls, then force that
  // choice.
  if (user_wants_controls_visible_.has_value()) {
    return *user_wants_controls_visible_;
  }

  if (FastHasAttribute(html_names::kControlsAttr) || IsFullscreen()) {
    return true;
  }

  ExecutionContext* context = GetExecutionContext();
  if (context && !context->CanExecuteScripts(kNotAboutToExecuteScript)) {
    return true;
  }
  return false;
}

bool HTMLMediaElement::ShouldShowAllControls() const {
  // If the user has explicitly shown or hidden the controls, then force that
  // choice. Otherwise returns whether controls should be shown and no controls
  // are meant to be hidden.
  return user_wants_controls_visible_.value_or(
      ShouldShowControls() && !ControlsListInternal()->CanShowAllControls());
}

DOMTokenList* HTMLMediaElement::controlsList() const {
  return controls_list_.Get();
}

HTMLMediaElementControlsList* HTMLMediaElement::ControlsListInternal() const {
  return controls_list_.Get();
}

void HTMLMediaElement::setVolume(double vol, ExceptionState& exception_state) {
  DVLOG(2) << "setVolume(" << static_cast<void*>(this) << ", " << vol << ")";

  if (volume_ == vol)
    return;

  if (RuntimeEnabledFeatures::MediaElementVolumeGreaterThanOneEnabled()) {
    if (vol < 0.0f) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kIndexSizeError,
          ExceptionMessages::IndexExceedsMinimumBound("volume", vol, 0.0));
      return;
    }
  } else if (vol < 0.0f || vol > 1.0f) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        ExceptionMessages::IndexOutsideRange(
            "volume", vol, 0.0, ExceptionMessages::kInclusiveBound, 1.0,
            ExceptionMessages::kInclusiveBound));
    return;
  }

  volume_ = vol;

  ScheduleNamedEvent(event_type_names::kVolumechange);
}

bool HTMLMediaElement::muted() const {
  // https://html.spec.whatwg.org/multipage/media.html#concept-media-muted
  if (RuntimeEnabledFeatures::MediaElementMutedDefaultStateEnabled() &&
      muted_is_default_) {
    return FastHasAttribute(html_names::kMutedAttr);
  }

  return muted_;
}

void HTMLMediaElement::setMuted(bool muted) {
  DVLOG(2) << "setMuted(" << static_cast<void*>(this) << ", "
           << base::ToString(muted) << ")";

  bool was_muted = this->muted();

  if (RuntimeEnabledFeatures::MediaElementMutedDefaultStateEnabled()) {
    muted_is_default_ = false;
  }
  muted_ = muted;

  if (was_muted == this->muted()) {
    return;
  }

  PseudoStateChanged(CSSSelector::kPseudoMuted);
  ScheduleNamedEvent(event_type_names::kVolumechange);
}

void HTMLMediaElement::SetUserWantsControlsVisible(bool visible) {
  user_wants_controls_visible_ = visible;
}

bool HTMLMediaElement::UserWantsControlsVisible() const {
  return user_wants_controls_visible_.value_or(false);
}

double HTMLMediaElement::EffectiveMediaVolume() const {
  if (muted()) {
    return 0;
  }

  return volume_;
}

AudioTrackList& HTMLMediaElement::audioTracks() {
  return *audio_tracks_;
}

void HTMLMediaElement::AudioTrackChanged(AudioTrack* track,
                                         TrackBase::ChangeSource) {
  DVLOG(3) << "audioTrackChanged(" << static_cast<void*>(this)
           << ") trackId= " << String(track->id())
           << " enabled=" << base::ToString(track->enabled());

  if (track->enabled()) {
    audioTracks().TrackEnabled(track->id(), track->IsExclusive());
  }

  audioTracks().ScheduleChangeEvent();
}

VideoTrackList& HTMLMediaElement::videoTracks() {
  return *video_tracks_;
}

void HTMLMediaElement::SelectedVideoTrackChanged(VideoTrack* track,
                                                 TrackBase::ChangeSource) {
  DVLOG(3) << "selectedVideoTrackChanged(" << static_cast<void*>(this)
           << ") selectedTrackId="
           << (track->selected() ? String(track->id()) : "none");

  if (track->selected())
    videoTracks().TrackSelected(track->id());

  videoTracks().ScheduleChangeEvent();
}

void HTMLMediaElement::ForgetResourceSpecificTracks() {
  audio_tracks_->RemoveAll();
  video_tracks_->RemoveAll();
}

TextTrack* HTMLMediaElement::addTextTrack(const V8TextTrackKind& kind,
                                          const AtomicString& label,
                                          const AtomicString& language,
                                          ExceptionState& exception_state) {
  // https://html.spec.whatwg.org/C/#dom-media-addtexttrack

  // 1-2. Create a new TextTrack object / text track.
  auto* text_track =
      MakeGarbageCollected<TextTrack>(kind, label, language, *this);
  text_track->SetReadinessState(TextTrack::kLoaded);

  // 3-4. Add the new text track to the media element's list of text tracks
  //    and queue an addtrack task.
  textTracks()->Append(text_track);

  // ..., its text track mode to the text track hidden mode, ...
  text_track->SetModeEnum(TextTrackMode::kHidden);

  // 5. Return the new TextTrack object.
  return text_track;
}

TextTrackList* HTMLMediaElement::textTracks() {
  if (!text_tracks_) {
    UseCounter::Count(GetDocument(), WebFeature::kMediaElementTextTrackList);
    text_tracks_ = MakeGarbageCollected<TextTrackList>(this);
  }

  return text_tracks_.Get();
}

void HTMLMediaElement::DidAddTrackElement(HTMLTrackElement* track_element) {
  // 4.8.12.11.3 Sourcing out-of-band text tracks
  TextTrack* text_track = track_element->track();
  if (!text_track)
    return;

  textTracks()->Append(text_track);

  // Do not schedule the track loading until parsing finishes so we don't start
  // before all tracks in the markup have been added.
  if (IsFinishedParsingChildren())
    ScheduleAutomaticTextTrackSelection();
}

void HTMLMediaElement::DidRemoveTrackElement(HTMLTrackElement* track_element) {
  TextTrack* text_track = track_element->track();
  if (!text_track)
    return;

  text_track->SetHasBeenConfigured(false);

  if (!text_tracks_)
    return;

  // 4.8.12.11.3 Sourcing out-of-band text tracks
  text_tracks_->Remove(text_track);

  wtf_size_t index =
      text_tracks_when_resource_selection_began_.Find(text_track);
  if (index != kNotFound)
    text_tracks_when_resource_selection_began_.EraseAt(index);
}

void HTMLMediaElement::HonorUserPreferencesForAutomaticTextTrackSelection() {
  if (!text_tracks_ || !text_tracks_->length())
    return;

  if (!should_perform_automatic_track_selection_)
    return;

  AutomaticTrackSelection::Configuration configuration;
  if (processing_preference_change_)
    configuration.disable_currently_enabled_tracks = true;
  if (text_tracks_visible_)
    configuration.force_enable_subtitle_or_caption_track = true;

  Settings* settings = GetDocument().GetSettings();
  if (settings) {
    configuration.text_track_kind_user_preference =
        settings->GetTextTrackKindUserPreference();
  }

  AutomaticTrackSelection track_selection(configuration);
  track_selection.Perform(*text_tracks_);
}

void HTMLMediaElement::AddPendingTextTracksFromCurrentList() {
  if (!text_tracks_) {
    return;
  }
  for (unsigned i = 0; i < text_tracks_->length(); ++i) {
    TextTrack* track = text_tracks_->AnonymousIndexedGetter(i);
    if (track->mode() == TextTrackMode::kDisabled) {
      continue;
    }
    if (track->GetReadinessState() != TextTrack::kLoading &&
        track->GetReadinessState() != TextTrack::kNotLoaded) {
      continue;
    }
    if (text_tracks_when_resource_selection_began_.Contains(track)) {
      continue;
    }
    text_tracks_when_resource_selection_began_.push_back(track);
  }
}

void HTMLMediaElement::SourceWasAdded(HTMLSourceElement*) {
  // Used to feed the <source>-child-walking resource-selection algorithm.
  // There is no such algorithm any more (see load()); <source> children are
  // still valid, parseable DOM nodes, they just aren't consulted for a URL.
}

void HTMLMediaElement::SourceWasRemoved(HTMLSourceElement*) {}

bool HTMLMediaElement::HasPendingActivity() const {
  return HasPendingActivityInternal();
}

bool HTMLMediaElement::HasPendingActivityInternal() const {
  // Wait for any pending events to be fired.
  return async_event_queue_->HasPendingEvents();
}

bool HTMLMediaElement::IsFullscreen() const {
  return Fullscreen::IsFullscreenElement(*this);
}

bool HTMLMediaElement::HasClosedCaptions() const {
  if (!text_tracks_)
    return false;

  for (unsigned i = 0; i < text_tracks_->length(); ++i) {
    if (text_tracks_->AnonymousIndexedGetter(i)->CanBeRendered())
      return true;
  }

  return false;
}

// static
void HTMLMediaElement::AssertShadowRootChildren(ShadowRoot& shadow_root) {
#if DCHECK_IS_ON()
  // MediaControls, the media-remoting interstitial and the
  // picture-in-picture interstitial are all gone; the only shadow child a
  // media element can have now is its TextTrackContainer.
  unsigned number_of_children = shadow_root.CountChildren();
  DCHECK_LE(number_of_children, 1u);
  if (number_of_children == 1) {
    DCHECK(shadow_root.firstChild()->IsTextTrackContainer());
  }
#endif
}

TextTrackContainer& HTMLMediaElement::EnsureTextTrackContainer() {
  UseCounter::Count(GetDocument(), WebFeature::kMediaElementTextTrackContainer);

  ShadowRoot& shadow_root = EnsureUserAgentShadowRoot();
  AssertShadowRootChildren(shadow_root);

  Node* first_child = shadow_root.firstChild();
  if (auto* first_child_text_track = DynamicTo<TextTrackContainer>(first_child))
    return *first_child_text_track;

  auto* text_track_container = MakeGarbageCollected<TextTrackContainer>(*this);
  shadow_root.InsertBefore(text_track_container, first_child);

  AssertShadowRootChildren(shadow_root);

  return *text_track_container;
}

void HTMLMediaElement::UpdateTextTrackDisplay() {
  DVLOG(3) << "updateTextTrackDisplay(" << static_cast<void*>(this) << ")";

  EnsureTextTrackContainer().UpdateDisplay(
      *this, TextTrackContainer::kDidNotStartExposingControls);
}

// static
void HTMLMediaElement::SetTextTrackKindUserPreferenceForAllMediaElements(
    Document* document) {
  auto it = DocumentToElementSetMap().find(document);
  if (it == DocumentToElementSetMap().end())
    return;
  DCHECK(it->value);
  WeakMediaElementSet& elements = *it->value;
  for (const auto& element : elements)
    element->AutomaticTrackSelectionForUpdatedUserPreference();
}

void HTMLMediaElement::AutomaticTrackSelectionForUpdatedUserPreference() {
  if (!text_tracks_ || !text_tracks_->length())
    return;

  MarkCaptionAndSubtitleTracksAsUnconfigured();
  processing_preference_change_ = true;
  text_tracks_visible_ = false;
  HonorUserPreferencesForAutomaticTextTrackSelection();
  processing_preference_change_ = false;

  // If a track is set to 'showing' post performing automatic track selection,
  // set text tracks state to visible to update the CC button and display the
  // track.
  text_tracks_visible_ = text_tracks_->HasShowingTracks();
  UpdateTextTrackDisplay();
}

void HTMLMediaElement::MarkCaptionAndSubtitleTracksAsUnconfigured() {
  if (!text_tracks_)
    return;

  for (unsigned i = 0; i < text_tracks_->length(); ++i) {
    TextTrack* text_track = text_tracks_->AnonymousIndexedGetter(i);
    if (text_track->IsVisualKind())
      text_track->SetHasBeenConfigured(false);
  }
}

bool HTMLMediaElement::IsURLAttribute(const Attribute& attribute) const {
  return attribute.GetName() == html_names::kSrcAttr ||
         HTMLElement::IsURLAttribute(attribute);
}

CueTimeline& HTMLMediaElement::GetCueTimeline() {
  if (!cue_timeline_)
    cue_timeline_ = MakeGarbageCollected<CueTimeline>(*this);
  return *cue_timeline_;
}

void HTMLMediaElement::ConfigureTextTrackDisplay() {
  DCHECK(text_tracks_);
  DVLOG(3) << "configureTextTrackDisplay(" << static_cast<void*>(this) << ")";

  if (processing_preference_change_)
    return;

  bool have_visible_text_track = text_tracks_->HasShowingTracks();
  text_tracks_visible_ = have_visible_text_track;

  if (!have_visible_text_track)
    return;

  UpdateTextTrackDisplay();
}

bool HTMLMediaElement::IsInteractiveContent() const {
  return FastHasAttribute(html_names::kControlsAttr);
}

FocusgroupFlags HTMLMediaElement::NativeArrowKeyAxes() const {
  // Media elements with controls use arrow keys for scrubbing (left/right)
  // and volume adjustment (up/down).
  if (ShouldShowControls()) {
    return FocusgroupFlags::kInline | FocusgroupFlags::kBlock;
  }
  return HTMLElement::NativeArrowKeyAxes();
}

void HTMLMediaElement::Trace(Visitor* visitor) const {
  visitor->Trace(async_event_queue_);
  visitor->Trace(error_);
  visitor->Trace(audio_tracks_);
  visitor->Trace(video_tracks_);
  visitor->Trace(cue_timeline_);
  visitor->Trace(text_tracks_);
  visitor->Trace(text_tracks_when_resource_selection_began_);
  visitor->Trace(controls_list_);
  Supplementable<HTMLMediaElement>::Trace(visitor);
  HTMLElement::Trace(visitor);
  ExecutionContextLifecycleStateObserver::Trace(visitor);
}

void HTMLMediaElement::ContextDestroyed() {
  DVLOG(3) << "contextDestroyed(" << static_cast<void*>(this) << ")";
  ForgetResourceSpecificTracks();
  UpdateLayoutObject();
}

void HTMLMediaElement::UpdateLayoutObject() {
  if (GetLayoutObject())
    GetLayoutObject()->UpdateFromElement();
}

}  // namespace blink
