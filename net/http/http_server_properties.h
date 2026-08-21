// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_SERVER_PROPERTIES_H_
#define NET_HTTP_HTTP_SERVER_PROPERTIES_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "base/containers/lru_cache.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/net_export.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/privacy_mode.h"
#include "net/http/alternative_service.h"
#include "net/http/broken_alternative_services.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_framer.h"  // TODO(willchan): Reconsider this.
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"
#include "url/scheme_host_port.h"

namespace base {
class Clock;
class TickClock;
}

namespace net {

class HttpServerPropertiesManager;
class IPAddress;
class NetLog;
struct SSLConfig;

struct NET_EXPORT ServerNetworkStats {
  ServerNetworkStats() = default;

  friend bool operator==(const ServerNetworkStats&,
                         const ServerNetworkStats&) = default;

  base::TimeDelta srtt;
};

typedef std::vector<AlternativeService> AlternativeServiceVector;

// Store at most 200 MRU RecentlyBrokenAlternativeServices in memory and disk.
// This ideally would be with the other constants in HttpServerProperties, but
// has to go here instead of prevent a circular dependency.
const int kMaxRecentlyBrokenAlternativeServiceEntries = 200;

// The interface for setting/retrieving the HTTP server properties.
// Currently, this class manages servers':
// * HTTP/2 support;
// * Alternative Service support;
// * ServerNetworkStats.
//
// Optionally retrieves and saves properties from/to disk. This class is not
// threadsafe.
class NET_EXPORT HttpServerProperties
    : public BrokenAlternativeServices::Delegate {
 public:
  // Store at most 500 MRU ServerInfos in memory and disk.
  static constexpr int kMaxServerInfoEntries = 500;
  // Max number of servers that can be recorded as requiring HTTP/1.1.
  static constexpr int kMaxServersRequiringHttp11Entries = 100;

  // Provides an interface to interact with persistent preferences storage
  // implemented by the embedder. The prefs are assumed not to have been loaded
  // before HttpServerPropertiesManager construction.
  class NET_EXPORT PrefDelegate {
   public:
    virtual ~PrefDelegate();

    // Returns the branch of the preferences system for the server properties.
    // Returns nullptr if the pref system has no data for the server properties.
    virtual const base::DictValue& GetServerProperties() const = 0;

    // Sets the server properties to the given value. If |callback| is
    // non-empty, flushes data to persistent storage and invokes |callback|
    // asynchronously when complete.
    virtual void SetServerProperties(base::DictValue dict,
                                     base::OnceClosure callback) = 0;

    // Starts listening for prefs to be loaded. If prefs are already loaded,
    // |pref_loaded_callback| will be invoked asynchronously. Callback will be
    // invoked even if prefs fail to load. Will only be called once by the
    // HttpServerPropertiesManager.
    virtual void WaitForPrefLoad(base::OnceClosure pref_loaded_callback) = 0;
  };

  // Contains metadata about a particular server. Note that all methods that
  // take a "SchemeHostPort" expect schemes of ws and wss to be mapped to http
  // and https, respectively. See GetNormalizedSchemeHostPort().
  struct NET_EXPORT ServerInfo {
    ServerInfo();
    ServerInfo(const ServerInfo& server_info);
    ServerInfo(ServerInfo&& server_info);
    ~ServerInfo();

    // Returns true if no fields are populated.
    bool empty() const;

    // Used in tests.
    bool operator==(const ServerInfo& other) const;

    // IMPORTANT:  When adding a field here, be sure to update
    // HttpServerProperties::OnServerInfoLoaded() as well as
    // HttpServerPropertiesManager to correctly load/save the from/to the pref
    // store.

    // Whether or not a server is known to support H2/SPDY. False indicates
    // known lack of support, true indicates known support, and not set
    // indicates unknown. The difference between false and not set only matters
    // when loading from disk, when an initialized false value will take
    // priority over a not set value.
    std::optional<bool> supports_spdy;

    std::optional<AlternativeServiceInfoVector> alternative_services;
    std::optional<ServerNetworkStats> server_network_stats;
  };

  struct NET_EXPORT ServerInfoMapKey {
    // If |use_network_anonymization_key| is false, an empty
    // NetworkAnonymizationKey is used instead of |network_anonymization_key|.
    // Note that |server| can be passed in via std::move(), since most callsites
    // can pass a recently created SchemeHostPort.
    ServerInfoMapKey(url::SchemeHostPort server,
                     const NetworkAnonymizationKey& network_anonymization_key,
                     bool use_network_anonymization_key);
    ~ServerInfoMapKey();

    bool operator<(const ServerInfoMapKey& other) const;

    // IMPORTANT: The constructor normalizes the scheme so that "ws" is replaced
    // by "http" and "wss" by "https", so this should never be compared directly
    // with values passed into to HttpServerProperties methods.
    url::SchemeHostPort server;

    NetworkAnonymizationKey network_anonymization_key;
  };

  class NET_EXPORT ServerInfoMap
      : public base::LRUCache<ServerInfoMapKey, ServerInfo> {
   public:
    ServerInfoMap();

    ServerInfoMap(const ServerInfoMap&) = delete;
    ServerInfoMap& operator=(const ServerInfoMap&) = delete;

    // If there's an entry corresponding to |key|, brings that entry to the
    // front and returns an iterator to it. Otherwise, inserts an empty
    // ServerInfo using |key|, and returns an iterator to it.
    iterator GetOrPut(const ServerInfoMapKey& key);

    // Erases the ServerInfo identified by |server_info_it| if no fields have
    // data. The iterator must point to an entry in the map. Regardless of
    // whether the entry is removed or not, returns iterator for the next entry.
    iterator EraseIfEmpty(iterator server_info_it);
  };

  // If a |pref_delegate| is specified, it will be used to read/write the
  // properties to a pref file. Writes are rate limited to improve performance.
  //
  // |tick_clock| is used for setting expiration times and scheduling the
  // expiration of broken alternative services. If null, default clock will be
  // used.
  //
  // |clock| is used for converting base::TimeTicks to base::Time for
  // wherever base::Time is preferable.
  explicit HttpServerProperties(
      std::unique_ptr<PrefDelegate> pref_delegate = nullptr,
      NetLog* net_log = nullptr,
      const base::TickClock* tick_clock = nullptr,
      base::Clock* clock = nullptr);

  HttpServerProperties(const HttpServerProperties&) = delete;
  HttpServerProperties& operator=(const HttpServerProperties&) = delete;

  ~HttpServerProperties() override;

  // Deletes all data. If |callback| is non-null, flushes data to disk
  // and invokes the callback asynchronously once changes have been written to
  // disk.
  void Clear(base::OnceClosure callback);

  // Returns true if `server`, in the context of `network_anonymization_key`,
  // has previously supported a network protocol which honors request
  // prioritization. `server` must have either http:// or https:// schemes.
  //
  // Note that this also implies that the server supports request
  // multiplexing, since priorities imply a relationship between
  // multiple requests.
  bool SupportsRequestPriority(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Returns the value set by SetSupportsSpdy(). If not set, returns false.
  // `server` must have either http:// or https:// schemes.
  bool GetSupportsSpdy(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Records whether |server| supports H2 or not. Information is restricted to
  // the context of |network_anonymization_key|, to prevent cross-site
  // information leakage.
  void SetSupportsSpdy(const url::SchemeHostPort& server,
                       const NetworkAnonymizationKey& network_anonymization_key,
                       bool supports_spdy);

  // Returns true if |server| has required HTTP/1.1 via HTTP/2 error code, in
  // the context of |network_anonymization_key|.
  bool RequiresHTTP11(const url::SchemeHostPort& server,
                      const NetworkAnonymizationKey& network_anonymization_key);

  // Require HTTP/1.1 on subsequent connections, in the context of
  // |network_anonymization_key|.  Not persisted.
  void SetHTTP11Required(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Modify SSLConfig to force HTTP/1.1 if necessary.
  void MaybeForceHTTP11(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key,
      SSLConfig* ssl_config);

  // Return all alternative services for |origin|, learned in the context of
  // |network_anonymization_key|, including broken ones. Returned alternative
  // services never have empty hostnames.
  AlternativeServiceInfoVector GetAlternativeServiceInfos(
      const url::SchemeHostPort& origin,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Set a single HTTP/2 alternative service for |origin|.  Previous
  // alternative services for |origin| are discarded.
  // |alternative_service.host| may be empty.
  void SetHttp2AlternativeService(
      const url::SchemeHostPort& origin,
      const NetworkAnonymizationKey& network_anonymization_key,
      const AlternativeService& alternative_service,
      base::Time expiration);

  // Set alternative services for |origin|, learned in the context of
  // |network_anonymization_key|.  Previous alternative services for |origin|
  // are discarded. Hostnames in |alternative_service_info_vector| may be empty.
  // |alternative_service_info_vector| may be empty.
  void SetAlternativeServices(
      const url::SchemeHostPort& origin,
      const NetworkAnonymizationKey& network_anonymization_key,
      const AlternativeServiceInfoVector& alternative_service_info_vector);

  // Marks |alternative_service| as broken in the context of
  // |network_anonymization_key|. |alternative_service.host| must not be empty.
  void MarkAlternativeServiceBroken(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Marks |alternative_service| as broken in the context of
  // |network_anonymization_key| until the default network changes.
  // |alternative_service.host| must not be empty.
  void MarkAlternativeServiceBrokenUntilDefaultNetworkChanges(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Marks |alternative_service| as recently broken in the context of
  // |network_anonymization_key|. |alternative_service.host| must not be empty.
  void MarkAlternativeServiceRecentlyBroken(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Returns true iff |alternative_service| is currently broken in the context
  // of |network_anonymization_key|. |alternative_service.host| must not be
  // empty.
  bool IsAlternativeServiceBroken(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key) const;

  // Returns true iff |alternative_service| was recently broken in the context
  // of |network_anonymization_key|. |alternative_service.host| must not be
  // empty.
  bool WasAlternativeServiceRecentlyBroken(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Confirms that |alternative_service| is working in the context of
  // |network_anonymization_key|. |alternative_service.host| must not be empty.
  void ConfirmAlternativeService(
      const AlternativeService& alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Called when the default network changes.
  // Clears all the alternative services that were marked broken until the
  // default network changed.
  void OnDefaultNetworkChanged();

  // Returns all alternative service mappings as human readable strings.
  // Empty alternative service hostnames will be printed as such.
  base::Value GetAlternativeServiceInfoAsValue() const;

  // Sets |stats| for |server|.
  void SetServerNetworkStats(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key,
      ServerNetworkStats stats);

  // Clears any stats for |server|.
  void ClearServerNetworkStats(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Returns any stats for |server| or nullptr if there are none.
  const ServerNetworkStats* GetServerNetworkStats(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // If values are present, sets initial_delay and
  // exponential_backoff_on_initial_delay which are used to calculate delay of
  // broken alternative services.
  void SetBrokenAlternativeServicesDelayParams(
      std::optional<base::TimeDelta> initial_delay,
      std::optional<bool> exponential_backoff_on_initial_delay);

  // Returns whether HttpServerProperties is initialized.
  bool IsInitialized() const;

  // BrokenAlternativeServices::Delegate method.
  void OnExpireBrokenAlternativeService(
      const AlternativeService& expired_alternative_service,
      const NetworkAnonymizationKey& network_anonymization_key) override;

  static base::TimeDelta GetUpdatePrefsDelayForTesting();

  // Test-only routines that call the methods used to load the specified
  // field(s) from a prefs file. Unlike OnPrefsLoaded(), these may be invoked
  // multiple times.
  void OnServerInfoLoadedForTesting(
      std::unique_ptr<ServerInfoMap> server_info_map) {
    OnServerInfoLoaded(std::move(server_info_map));
  }
  void OnBrokenAndRecentlyBrokenAlternativeServicesLoadedForTesting(
      std::unique_ptr<BrokenAlternativeServiceList>
          broken_alternative_service_list,
      std::unique_ptr<RecentlyBrokenAlternativeServices>
          recently_broken_alternative_services) {
    OnBrokenAndRecentlyBrokenAlternativeServicesLoaded(
        std::move(broken_alternative_service_list),
        std::move(recently_broken_alternative_services));
  }

  const std::string* GetCanonicalSuffixForTesting(
      const std::string& host) const {
    return GetCanonicalSuffix(host);
  }

  const ServerInfoMap& server_info_map_for_testing() const {
    return server_info_map_;
  }

  // This will invalidate the start-up properties if called before
  // initialization.
  void FlushWritePropertiesForTesting(base::OnceClosure callback);

  const BrokenAlternativeServices& broken_alternative_services_for_testing()
      const {
    return broken_alternative_services_;
  }

  // TODO(mmenke): Look into removing this.
  HttpServerPropertiesManager* properties_manager_for_testing() {
    return properties_manager_.get();
  }

 private:
  // TODO (wangyix): modify HttpServerProperties unit tests so this
  // friendness is no longer required.
  friend class HttpServerPropertiesPeer;

  using CanonicalMap = base::flat_map<ServerInfoMapKey, url::SchemeHostPort>;
  using CanonicalSuffixList = std::vector<std::string>;

  // Internal implementations of public methods. SchemeHostPort argument must be
  // normalized before calling (ws/wss replaced with http/https). Use wrapped
  // functions instead of putting the normalization in the public functions to
  // reduce chance of regression - normalization in ServerInfoMapKey's
  // constructor would leave |server.scheme| as wrong if not access through the
  // key, and explicit normalization to create |normalized_server| means the one
  // with the incorrect scheme would still be available.
  bool RequiresHTTP11Internal(
      url::SchemeHostPort server,
      const NetworkAnonymizationKey& network_anonymization_key);
  void SetHTTP11RequiredInternal(
      url::SchemeHostPort server,
      const NetworkAnonymizationKey& network_anonymization_key);
  AlternativeServiceInfoVector GetAlternativeServiceInfosInternal(
      const url::SchemeHostPort& origin,
      const NetworkAnonymizationKey& network_anonymization_key);
  void SetAlternativeServicesInternal(
      const url::SchemeHostPort& origin,
      const NetworkAnonymizationKey& network_anonymization_key,
      const AlternativeServiceInfoVector& alternative_service_info_vector);
  void SetServerNetworkStatsInternal(
      url::SchemeHostPort server,
      const NetworkAnonymizationKey& network_anonymization_key,
      ServerNetworkStats stats);
  void ClearServerNetworkStatsInternal(
      url::SchemeHostPort server,
      const NetworkAnonymizationKey& network_anonymization_key);
  const ServerNetworkStats* GetServerNetworkStatsInternal(
      url::SchemeHostPort server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Helper function to use the passed in parameters and
  // |use_network_anonymization_key_| to create a ServerInfoMapKey.
  ServerInfoMapKey CreateServerInfoKey(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key) const;

  // Return the iterator for |server| in the context of
  // |network_anonymization_key|, or for its canonical host, or end. Skips over
  // ServerInfos without |alternative_service_info| populated.
  ServerInfoMap::const_iterator GetIteratorWithAlternativeServiceInfo(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Return the known alternative service host for |server|, or std::nullopt if
  // none exists.
  std::optional<AlternativeService> GetKnownAltSvcHost(
      const url::SchemeHostPort& server) const;

  // Return the canonical host for |server|  in the context of
  // |network_anonymization_key|, or end if none exists.
  CanonicalMap::const_iterator GetCanonicalAltSvcHost(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key) const;

  // Remove the canonical alt-svc host for |server| with
  // |network_anonymization_key|.
  void RemoveAltSvcCanonicalHost(
      const url::SchemeHostPort& server,
      const NetworkAnonymizationKey& network_anonymization_key);

  // Returns the canonical host suffix for |host|, or nullptr if none
  // exists.
  const std::string* GetCanonicalSuffix(const std::string& host) const;

  void OnPrefsLoaded(std::unique_ptr<ServerInfoMap> server_info_map,
                     std::unique_ptr<BrokenAlternativeServiceList>
                         broken_alternative_service_list,
                     std::unique_ptr<RecentlyBrokenAlternativeServices>
                         recently_broken_alternative_services);

  // These methods are called by OnPrefsLoaded to handle merging properties
  // loaded from prefs with what has been learned while waiting for prefs to
  // load.
  void OnServerInfoLoaded(std::unique_ptr<ServerInfoMap> server_info_map);
  void OnBrokenAndRecentlyBrokenAlternativeServicesLoaded(
      std::unique_ptr<BrokenAlternativeServiceList>
          broken_alternative_service_list,
      std::unique_ptr<RecentlyBrokenAlternativeServices>
          recently_broken_alternative_services);

  // Queue a delayed call to WriteProperties(). If |is_initialized_| is false,
  // or |properties_manager_| is nullptr, or there's already a queued call to
  // WriteProperties(), does nothing.
  void MaybeQueueWriteProperties();

  // Writes cached state to |properties_manager_|, which must not be null.
  // Invokes |callback| on completion, if non-null.
  void WriteProperties(base::OnceClosure callback) const;

  raw_ptr<const base::TickClock> tick_clock_;  // Unowned
  raw_ptr<base::Clock> clock_;                 // Unowned

  // Cached value of whether network state partitioning is enabled. Cached to
  // improve performance.
  const bool use_network_anonymization_key_;

  // Set to true once initial properties have been retrieved from disk by
  // |properties_manager_|. Always true if |properties_manager_| is nullptr.
  bool is_initialized_;

  // Queue a write when resources finish loading. Set to true when
  // MaybeQueueWriteProperties() is invoked while still waiting on
  // initialization to complete.
  bool queue_write_on_load_ = false;

  // Used to load/save properties from/to preferences. May be nullptr.
  std::unique_ptr<HttpServerPropertiesManager> properties_manager_;

  ServerInfoMap server_info_map_;

  // Set of servers that require HTTP/1.1. Not persisted to disk. This is
  // separate from ServerInfoMap because it's generally empty, and has to be
  // checked on every network request, rather than only when establishing
  // connections.
  base::LRUCacheSet<ServerInfoMapKey> servers_requiring_http_11_{
      kMaxServersRequiringHttp11Entries};

  BrokenAlternativeServices broken_alternative_services_;

  // Contains a map of servers which could share the same alternate protocol.
  // Map from a Canonical scheme/host/port/NAK (host is some postfix of host
  // names) to an actual origin, which has a plausible alternate protocol
  // mapping.
  CanonicalMap canonical_alt_svc_map_;

  // Contains list of suffixes (for example ".c.youtube.com",
  // ".googlevideo.com", ".googleusercontent.com") of canonical hostnames.
  const CanonicalSuffixList canonical_suffixes_;


  // Used to post calls to WriteProperties().
  base::OneShotTimer prefs_update_timer_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_SERVER_PROPERTIES_H_
