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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_

// This build has no media playback pipeline: //media's demux/decode stack,
// blink's platform/audio (WebAudio), platform/media (WebMediaPlayerClient,
// WebAudioSourceProviderClient) and the media::mojom MediaPlayer/
// MediaPlayerHost/MediaPlayerObserver mojo interfaces were all cut. There is
// therefore no WebMediaPlayer implementation that could ever be created for
// a <video>/<audio> element.
//
// HTMLMediaElement still exists as the DOM element the HTML parser needs for
// <video>/<audio>/<source>/<track> markup to produce a correct tree, and as
// the LayoutObject-owning element that HTMLVideoElement's poster image
// paints through (see LayoutVideo, VideoPainter). Every part of the old
// class that existed to drive a player -- resource selection, network/ready
// state transitions, seeking, playback rate, autoplay, media source
// extensions, remote playback, picture-in-picture, WebAudio source-node
// wiring, mojo bindings -- has been deleted rather than stubbed. What is
// left reports the honest, permanent state of an element with no playback
// backend: networkState stays NETWORK_EMPTY, readyState stays HAVE_NOTHING,
// paused stays true, and the show-poster flag stays set so a <video
// poster="x.jpg"> renders its poster image, which is the only pixels a
// <video> should ever produce in a static screenshot.
//
// The text track subsystem (core/html/track/**) is untouched: it does not
// depend on WebMediaPlayer, so it keeps compiling and behaving exactly as
// before. It just never has a moving playback clock to drive active-cue
// rendering, since currentTime() never advances.

#include <limits>
#include <optional>

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_state_observer.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/track/track_base.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/prefinalizer.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cancellable_task.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace cc {
class Layer;
}

namespace blink {

class AudioTrack;
class AudioTrackList;
class ContentType;
class CueTimeline;
class Event;
class EventQueue;
class ExceptionState;
class HTMLMediaElementControlsList;
class HTMLSourceElement;
class HTMLTrackElement;
class MediaError;
class TextTrack;
class TextTrackContainer;
class TextTrackList;
class TimeRanges;
class VideoTrack;
class VideoTrackList;
class V8CanPlayTypeResult;
class V8TextTrackKind;
class WebMediaPlayer;

class CORE_EXPORT HTMLMediaElement
    : public HTMLElement,
      public Supplementable<HTMLMediaElement>,
      public ActiveScriptWrappable<HTMLMediaElement>,
      public ExecutionContextLifecycleStateObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_PRE_FINALIZER(HTMLMediaElement, Dispose);

 public:
  // Limits the range of media playback rate.
  static constexpr double kMinPlaybackRate = 0.0625;
  static constexpr double kMaxPlaybackRate = 16.0;

  bool IsMediaElement() const override { return true; }

  static MIMETypeRegistry::SupportsType GetSupportsType(const ContentType&);

  static bool IsHLSURL(const KURL&);

  // Called by Page when the "media controls enabled" setting changes. Used
  // to notify each element's MediaControls widget; MediaControls no longer
  // exists, so this is a no-op kept only so core/page/page.cc's call site
  // keeps compiling.
  static void OnMediaControlsEnabledChange(Document*);

  void Trace(Visitor*) const override;

  // There is no media playback pipeline in this build, so there is no
  // WebMediaPlayer to return; see the file comment.
  WebMediaPlayer* GetWebMediaPlayer() const { return nullptr; }

  // Returns true if the loaded media has a video/audio track. Always false:
  // without a player nothing is ever demuxed, so no track is ever reported.
  bool HasVideo() const { return false; }
  bool HasAudio() const { return false; }

  // Whether the media element has encrypted audio or video streams. Always
  // false: EME (media::mojom Cdm*) is part of the deleted playback pipeline.
  bool IsEncrypted() const { return false; }

  bool SupportsSave() const;
  bool SupportsLoop() const;

  cc::Layer* CcLayer() const { return cc_layer_; }
  void SetCcLayerForTesting(cc::Layer* layer) { cc_layer_ = layer; }

  void ScheduleAutomaticTextTrackSelection();

  // error state
  MediaError* error() const;

  // network state
  void SetSrc(const AtomicString&);
  const KURL& currentSrc() const { return current_src_; }

  enum NetworkState {
    kNetworkEmpty,
    kNetworkIdle,
    kNetworkLoading,
    kNetworkNoSource
  };
  NetworkState getNetworkState() const { return network_state_; }

  String preload() const;
  void setPreload(const AtomicString&);

  // Returns true if the element has a src attribute or <source> child
  // elements that could provide media.
  bool HasMediaSources() const;

  TimeRanges* buffered() const;
  void load();
  V8CanPlayTypeResult canPlayType(const String& mime_type) const;

  // ready state
  enum ReadyState {
    kHaveNothing,
    kHaveMetadata,
    kHaveCurrentData,
    kHaveFutureData,
    kHaveEnoughData
  };
  ReadyState getReadyState() const { return ready_state_; }
  bool seeking() const { return false; }

  // The seek algorithm (Seek(), FinishSeek()) was cut with the rest of the
  // resource-selection code: there is no player to seek within, and
  // setCurrentTime() now just stores the number script asked for instead of
  // running a seek. Nothing ever updates a "last seek time" as a result, so
  // this permanently reports the field's untouched initial value. Used by
  // CueTimeline::TimeMarchesOn() to distinguish a seek from monotonic
  // playback; since neither a seek nor real playback ever happens here, the
  // distinction is moot, but the comparison it feeds stays well-defined.
  double LastSeekTime() const { return 0; }

  // playback state
  double currentTime() const { return current_time_; }
  void setCurrentTime(double);
  double duration() const { return duration_; }
  bool paused() const { return paused_; }
  double defaultPlaybackRate() const { return default_playback_rate_; }
  void setDefaultPlaybackRate(double);
  double playbackRate() const { return playback_rate_; }
  void setPlaybackRate(double, ExceptionState& = ASSERT_NO_EXCEPTION);
  TimeRanges* played();
  TimeRanges* seekable() const;
  bool ended() const;
  bool Autoplay() const;
  bool Loop() const;
  void SetLoop(bool);
  std::optional<DOMExceptionCode> Play();

  void pause();
  double latencyHint() const;
  void setLatencyHint(double);
  bool preservesPitch() const { return preserves_pitch_; }
  void setPreservesPitch(bool);

  // statistics -- always 0, since nothing is ever decoded.
  uint64_t webkitAudioDecodedByteCount() const { return 0; }
  uint64_t webkitVideoDecodedByteCount() const { return 0; }

  // controls
  bool ShouldShowControls() const;
  bool ShouldShowAllControls() const;
  DOMTokenList* controlsList() const;
  HTMLMediaElementControlsList* ControlsListInternal() const;
  double volume() const { return volume_; }
  void setVolume(double, ExceptionState& = ASSERT_NO_EXCEPTION);
  bool muted() const;
  void setMuted(bool);
  virtual bool SupportsPictureInPicture() const { return false; }
  void SetUserWantsControlsVisible(bool visible);
  bool UserWantsControlsVisible() const;

  AudioTrackList& audioTracks();
  void AudioTrackChanged(AudioTrack*, TrackBase::ChangeSource);

  VideoTrackList& videoTracks();
  void SelectedVideoTrackChanged(VideoTrack*, TrackBase::ChangeSource);

  TextTrack* addTextTrack(const V8TextTrackKind& kind,
                          const AtomicString& label,
                          const AtomicString& language,
                          ExceptionState&);

  TextTrackList* textTracks();
  CueTimeline& GetCueTimeline();

  // Implements the "forget the media element's media-resource-specific
  // tracks" algorithm in the HTML5 spec.
  void ForgetResourceSpecificTracks();

  void DidAddTrackElement(HTMLTrackElement*);
  void DidRemoveTrackElement(HTMLTrackElement*);

  void HonorUserPreferencesForAutomaticTextTrackSelection();

  // Implements the "populate the list of pending text tracks" step of the
  // resource selection algorithm. Idempotently adds non-disabled,
  // still-loading tracks to the snapshot used by TextTracksAreReady().
  void AddPendingTextTracksFromCurrentList();

  bool TextTracksAreReady() const;
  void ConfigureTextTrackDisplay();
  void UpdateTextTrackDisplay();

  void TextTrackReadyStateChanged(TextTrack*);

  void TextTrackModeChanged(TextTrack*);
  void DisableAutomaticTextTrackSelection();

  // EventTarget function.
  // Both Node (via HTMLElement) and ExecutionContextLifecycleStateObserver
  // define this method, which causes an ambiguity error at compile time. This
  // class's constructor ensures that both implementations return document, so
  // return the result of one of them here.
  using HTMLElement::GetExecutionContext;

  bool IsFullscreen() const;

  bool HasClosedCaptions() const;
  bool TextTracksVisible() const { return text_tracks_visible_; }

  static void SetTextTrackKindUserPreferenceForAllMediaElements(Document*);
  void AutomaticTrackSelectionForUpdatedUserPreference();

  void SourceWasRemoved(HTMLSourceElement*);
  void SourceWasAdded(HTMLSourceElement*);

  // ScriptWrappable functions.
  bool HasPendingActivity() const override;

  // Checks to see if current media data is CORS-same-origin. Always true:
  // nothing is ever loaded, so there is nothing to taint.
  bool IsMediaDataCorsSameOrigin() const { return true; }

  void ScheduleEvent(Event*);
  void ScheduleNamedEvent(const AtomicString& event_name);

  // Returns the "effective media volume" value as specified in the HTML5
  // spec.
  double EffectiveMediaVolume() const;

  // Predicates also used when dispatching wrapper creation (cf.
  // [SpecialWrapFor] IDL attribute usage.)
  virtual bool IsHTMLAudioElement() const { return false; }
  virtual bool IsHTMLVideoElement() const { return false; }

  // Predicates for CSS pseudo-classes that have non-trivial conditions or
  // that aren't exposed by any other method. (Simple pseudos like :paused
  // don't have dedicated helpers.) Both are permanently false: buffering and
  // stalling only happen while a player is loading, and there is none.
  bool MatchesBufferingPseudo() const { return false; }
  bool MatchesStalledPseudo() const { return false; }

  bool IsShowPosterFlagSet() const { return show_poster_flag_; }

 protected:
  // Assert the correct order of the children in shadow dom when DCHECK is on.
  static void AssertShadowRootChildren(ShadowRoot&);

  HTMLMediaElement(const QualifiedName&, Document&);
  ~HTMLMediaElement() override;
  void Dispose();

  void ParseAttribute(const AttributeModificationParams&) override;
  void FinishParsingChildren() final;
  bool IsURLAttribute(const Attribute&) const override;
  void AttachLayoutTree(AttachContext&) override;
  void CloneNonAttributePropertiesFrom(const Element&,
                                       NodeCloningData&) override;

  InsertionNotificationRequest InsertedInto(ContainerNode&) override;
  void RemovedFrom(ContainerNode&) override;

  void DidMoveToNewDocument(Document& old_document) override;
  virtual KURL PosterImageURL() const { return KURL(); }

  void UpdateLayoutObject();

 private:
  // Friend class for testing.
  friend class ContextMenuControllerTest;
  friend class HTMLMediaElementTest;

  bool HasPendingActivityInternal() const;

  bool AlwaysCreateUserAgentShadowRoot() const final { return true; }

  FocusableState SupportsFocus(UpdateBehavior update_behavior) const final;
  FocusableState IsFocusableState(UpdateBehavior update_behavior) const final;
  int DefaultTabIndex() const final;
  bool LayoutObjectIsNeeded(const DisplayStyle&) const override;
  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;
  void DidNotifySubtreeInsertionsToDocument() override {}
  void DidRecalcStyle(const StyleRecalcChange) final;

  bool CanStartSelection() const override { return false; }

  bool IsInteractiveContent() const final;
  FocusgroupFlags NativeArrowKeyAxes() const final;

  // ExecutionContextLifecycleStateObserver functions.
  void ContextLifecycleStateChanged(mojom::blink::FrameLifecycleState) override {}
  void ContextDestroyed() override;

  void MarkCaptionAndSubtitleTracksAsUnconfigured();

  TextTrackContainer& EnsureTextTrackContainer();

  friend class Internals;
  friend class TrackDisplayUpdateScope;
  friend class HTMLMediaElementTest;
  friend class HTMLMediaElementEventListenersTest;
  friend class HTMLVideoElement;
  friend class HTMLVideoElementTest;

  Member<EventQueue> async_event_queue_;

  double playback_rate_ = 1.0;
  double default_playback_rate_ = 1.0;
  NetworkState network_state_ = kNetworkEmpty;
  ReadyState ready_state_ = kHaveNothing;

  // The URL a real resource-selection algorithm would have chosen. Since
  // that algorithm never runs (there is no player to feed), this stays
  // empty: no source is ever selected.
  KURL current_src_;

  // To prevent potential regression when extended by the MSE API, do not set
  // |error_| outside of constructor and SetError().
  Member<MediaError> error_;

  double volume_ = 1.0;

  // What script last asked currentTime to be. There is no decoder to seek,
  // so this is just the number JS reads back; it never advances on its own.
  double current_time_ = 0;

  // Cached duration; always NaN because no media is ever loaded.
  double duration_ = std::numeric_limits<double>::quiet_NaN();

  cc::Layer* cc_layer_ = nullptr;

  bool should_perform_automatic_track_selection_ : 1 = true;
  bool text_tracks_visible_ : 1 = false;
  bool processing_preference_change_ : 1 = false;

  bool muted_is_default_ : 1 = true;
  bool muted_ : 1 = false;
  bool paused_ : 1 = true;
  bool show_poster_flag_ : 1 = true;

  // Set if the user has used the context menu to set the visibility of the
  // controls.
  std::optional<bool> user_wants_controls_visible_;

  // Whether |web_media_player_| should apply pitch adjustments at playback
  // rates other than 1.0. Stored only so the IDL getter/setter round-trips;
  // there is no player to apply it to.
  bool preserves_pitch_ = true;

  Member<AudioTrackList> audio_tracks_;
  Member<VideoTrackList> video_tracks_;
  Member<TextTrackList> text_tracks_;
  HeapVector<Member<TextTrack>> text_tracks_when_resource_selection_began_;

  Member<CueTimeline> cue_timeline_;

  // Coalesces automatic text track selection into a single task; see
  // ScheduleAutomaticTextTrackSelection().
  TaskHandle text_track_selection_task_handle_;

  Member<HTMLMediaElementControlsList> controls_list_;
};

template <>
struct DowncastTraits<HTMLMediaElement> {
  static bool AllowFrom(const Node& node) {
    auto* html_element = DynamicTo<HTMLElement>(node);
    return html_element && AllowFrom(*html_element);
  }
  static bool AllowFrom(const HTMLElement& html_element) {
    return html_element.HasTagName(html_names::kAudioTag) ||
           html_element.HasTagName(html_names::kVideoTag);
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_MEDIA_ELEMENT_H_
