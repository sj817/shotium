// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_server_properties_manager.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/containers/adapters.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/port_util.h"
#include "net/base/privacy_mode.h"
#include "net/http/http_server_properties.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

// "version" 0 indicates, http_server_properties doesn't have "version"
// property.
const int kMissingVersion = 0;

// The version number of persisted http_server_properties.
const int kVersionNumber = 5;

// Persist at most 200 currently-broken alternative services to disk.
const int kMaxBrokenAlternativeServicesToPersist = 200;

const char kServerKey[] = "server";
const char kNetworkAnonymizationKey[] = "anonymization";
const char kVersionKey[] = "version";
const char kServersKey[] = "servers";
const char kSupportsSpdyKey[] = "supports_spdy";
const char kServerInfoKey[] = "server_info";
const char kAddressKey[] = "address";
const char kAlternativeServiceKey[] = "alternative_service";
const char kProtocolKey[] = "protocol_str";
const char kHostKey[] = "host";
const char kPortKey[] = "port";
const char kExpirationKey[] = "expiration";
const char kAdvertisedAlpnsKey[] = "advertised_alpns";
const char kNetworkStatsKey[] = "network_stats";
const char kSrttKey[] = "srtt";
const char kBrokenAlternativeServicesKey[] = "broken_alternative_services";
const char kBrokenUntilKey[] = "broken_until";
const char kBrokenCountKey[] = "broken_count";

// Utility method to return only those AlternativeServiceInfos that should be
// persisted to disk. In particular, removes expired and invalid alternative
// services. Also checks if an alternative service for the same canonical suffix
// has already been saved, and if so, returns an empty list.
AlternativeServiceInfoVector GetAlternativeServiceToPersist(
    const std::optional<AlternativeServiceInfoVector>& alternative_services,
    const HttpServerProperties::ServerInfoMapKey& server_info_key,
    base::Time now,
    const HttpServerPropertiesManager::GetCannonicalSuffix&
        get_canonical_suffix,
    std::set<std::pair<std::string, NetworkAnonymizationKey>>*
        persisted_canonical_suffix_set) {
  if (!alternative_services)
    return AlternativeServiceInfoVector();
  // Separate out valid, non-expired AlternativeServiceInfo entries.
  AlternativeServiceInfoVector notbroken_alternative_service_info_vector;
  for (const auto& alternative_service_info : alternative_services.value()) {
    if (alternative_service_info.expiration() < now ||
        !IsAlternateProtocolValid(
            alternative_service_info.alternative_service().protocol)) {
      continue;
    }
    notbroken_alternative_service_info_vector.push_back(
        alternative_service_info);
  }
  if (notbroken_alternative_service_info_vector.empty())
    return notbroken_alternative_service_info_vector;
  const std::string* canonical_suffix =
      get_canonical_suffix.Run(server_info_key.server.host());
  if (canonical_suffix) {
    // Don't save if have already saved information associated with the same
    // canonical suffix.
    std::pair<std::string, NetworkAnonymizationKey> index(
        *canonical_suffix, server_info_key.network_anonymization_key);
    if (persisted_canonical_suffix_set->find(index) !=
        persisted_canonical_suffix_set->end()) {
      return AlternativeServiceInfoVector();
    }
    persisted_canonical_suffix_set->emplace(std::move(index));
  }
  return notbroken_alternative_service_info_vector;
}

void AddAlternativeServiceFieldsToDictionaryValue(
    const AlternativeService& alternative_service,
    base::DictValue& dict) {
  dict.Set(kPortKey, alternative_service.port);
  if (!alternative_service.host.empty()) {
    dict.Set(kHostKey, alternative_service.host);
  }
  dict.Set(kProtocolKey, NextProtoToString(alternative_service.protocol));
}

// Fails in the case of NetworkAnonymizationKeys that can't be persisted to
// disk, like unique origins.
bool TryAddBrokenAlternativeServiceFieldsToDictionaryValue(
    const BrokenAlternativeService& broken_alt_service,
    base::DictValue& dict) {
  base::Value network_anonymization_key_value;
  if (!broken_alt_service.network_anonymization_key.ToValue(
          &network_anonymization_key_value)) {
    return false;
  }

  dict.Set(kNetworkAnonymizationKey,
           std::move(network_anonymization_key_value));
  AddAlternativeServiceFieldsToDictionaryValue(
      broken_alt_service.alternative_service, dict);
  return true;
}

bool GetNetworkAnonymizationKeyFromDict(
    const base::DictValue& dict,
    bool use_network_anonymization_key,
    NetworkAnonymizationKey* out_network_anonymization_key) {
  const base::Value* network_anonymization_key_value =
      dict.Find(kNetworkAnonymizationKey);
  NetworkAnonymizationKey network_anonymization_key;
  if (!network_anonymization_key_value ||
      !NetworkAnonymizationKey::FromValue(*network_anonymization_key_value,
                                          &network_anonymization_key)) {
    return false;
  }

  // Fail if NetworkAnonymizationKeys are disabled, but the entry has a
  // non-empty NetworkAnonymizationKey.
  if (!use_network_anonymization_key && !network_anonymization_key.IsEmpty())
    return false;

  *out_network_anonymization_key = std::move(network_anonymization_key);
  return true;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
//  HttpServerPropertiesManager

HttpServerPropertiesManager::HttpServerPropertiesManager(
    std::unique_ptr<HttpServerProperties::PrefDelegate> pref_delegate,
    OnPrefsLoadedCallback on_prefs_loaded_callback,
    NetLog* net_log,
    const base::TickClock* clock)
    : pref_delegate_(std::move(pref_delegate)),
      on_prefs_loaded_callback_(std::move(on_prefs_loaded_callback)),
      clock_(clock),
      net_log_(
          NetLogWithSource::Make(net_log,
                                 NetLogSourceType::HTTP_SERVER_PROPERTIES)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(pref_delegate_);
  DCHECK(on_prefs_loaded_callback_);
  DCHECK(clock_);

  pref_delegate_->WaitForPrefLoad(
      base::BindOnce(&HttpServerPropertiesManager::OnHttpServerPropertiesLoaded,
                     pref_load_weak_ptr_factory_.GetWeakPtr()));
  net_log_.BeginEvent(NetLogEventType::HTTP_SERVER_PROPERTIES_INITIALIZATION);
}

HttpServerPropertiesManager::~HttpServerPropertiesManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void HttpServerPropertiesManager::ReadPrefs(
    std::unique_ptr<HttpServerProperties::ServerInfoMap>* server_info_map,
    std::unique_ptr<BrokenAlternativeServiceList>*
        broken_alternative_service_list,
    std::unique_ptr<RecentlyBrokenAlternativeServices>*
        recently_broken_alternative_services) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  net_log_.EndEvent(NetLogEventType::HTTP_SERVER_PROPERTIES_INITIALIZATION);

  const base::DictValue& http_server_properties_dict =
      pref_delegate_->GetServerProperties();

  net_log_.AddEvent(NetLogEventType::HTTP_SERVER_PROPERTIES_UPDATE_CACHE,
                    [&] { return http_server_properties_dict.Clone(); });
  std::optional<int> maybe_version_number =
      http_server_properties_dict.FindInt(kVersionKey);
  if (!maybe_version_number.has_value() ||
      *maybe_version_number != kVersionNumber) {
    DVLOG(1) << "Missing or unsupported. Clearing all properties. "
             << maybe_version_number.value_or(kMissingVersion);
    return;
  }

  // For Version 5, data is stored in the following format.
  // `servers` are saved in LRU order (least-recently-used item is in the
  // front). `servers` are in the format flattened representation of
  // (scheme/host/port) where port might be ignored if is default with scheme.
  //
  // "http_server_properties": {
  //      "servers": [
  //          {"https://yt3.ggpht.com" : {...}},
  //          {"http://0.client-channel.google.com:443" : {...}},
  //          {"http://0-edge-chat.facebook.com" : {...}},
  //          ...
  //      ], ...
  // },
  const base::ListValue* servers_list =
      http_server_properties_dict.FindList(kServersKey);
  if (!servers_list) {
    DVLOG(1) << "Malformed http_server_properties for servers list.";
    return;
  }

  *server_info_map = std::make_unique<HttpServerProperties::ServerInfoMap>();

  bool use_network_anonymization_key =
      NetworkAnonymizationKey::IsPartitioningEnabled();

  // Iterate `servers_list` (least-recently-used item is in the front) so that
  // entries are inserted into `server_info_map` from oldest to newest.
  for (const auto& server_dict_value : *servers_list) {
    if (!server_dict_value.is_dict()) {
      DVLOG(1) << "Malformed http_server_properties for servers dictionary.";
      continue;
    }
    AddServerData(server_dict_value.GetDict(), server_info_map->get(),
                  use_network_anonymization_key);
  }

  // Read list containing broken and recently-broken alternative services, if
  // it exists.
  const base::ListValue* broken_alt_svc_list =
      http_server_properties_dict.FindList(kBrokenAlternativeServicesKey);
  if (broken_alt_svc_list && base::FeatureList::IsEnabled(
                                 features::kPersistBrokenAlternativeServices)) {
    *broken_alternative_service_list =
        std::make_unique<BrokenAlternativeServiceList>();
    *recently_broken_alternative_services =
        std::make_unique<RecentlyBrokenAlternativeServices>(
            kMaxRecentlyBrokenAlternativeServiceEntries);

    // Iterate `broken_alt_svc_list` (least-recently-used item is in the front)
    // so that entries are inserted into `recently_broken_alternative_services`
    // from oldest to newest.
    for (const auto& broken_alt_svc_entry_dict_value : *broken_alt_svc_list) {
      if (!broken_alt_svc_entry_dict_value.is_dict()) {
        DVLOG(1) << "Malformed broken alterantive service entry.";
        continue;
      }
      AddToBrokenAlternativeServices(
          broken_alt_svc_entry_dict_value.GetDict(),
          use_network_anonymization_key, broken_alternative_service_list->get(),
          recently_broken_alternative_services->get());
    }
  }

  // Set the properties loaded from prefs on |http_server_properties_impl_|.

  if (*recently_broken_alternative_services) {
    DCHECK(*broken_alternative_service_list);

    UMA_HISTOGRAM_COUNTS_1000("Net.CountOfBrokenAlternativeServices",
                              (*broken_alternative_service_list)->size());
    UMA_HISTOGRAM_COUNTS_1000("Net.CountOfRecentlyBrokenAlternativeServices",
                              (*recently_broken_alternative_services)->size());
  }
}

void HttpServerPropertiesManager::AddToBrokenAlternativeServices(
    const base::DictValue& broken_alt_svc_entry_dict,
    bool use_network_anonymization_key,
    BrokenAlternativeServiceList* broken_alternative_service_list,
    RecentlyBrokenAlternativeServices* recently_broken_alternative_services) {
  AlternativeService alt_service;
  if (!ParseAlternativeServiceDict(broken_alt_svc_entry_dict, false,
                                   "broken alternative services",
                                   &alt_service)) {
    return;
  }

  NetworkAnonymizationKey network_anonymization_key;
  if (!GetNetworkAnonymizationKeyFromDict(broken_alt_svc_entry_dict,
                                          use_network_anonymization_key,
                                          &network_anonymization_key)) {
    return;
  }

  // Each entry must contain either broken-count and/or broken-until fields.
  bool contains_broken_count_or_broken_until = false;

  // Read broken-count and add an entry for |alt_service| into
  // |recently_broken_alternative_services|.
  if (broken_alt_svc_entry_dict.Find(kBrokenCountKey)) {
    std::optional<int> broken_count =
        broken_alt_svc_entry_dict.FindInt(kBrokenCountKey);
    if (!broken_count.has_value()) {
      DVLOG(1) << "Recently broken alternative service has malformed "
               << "broken-count.";
      return;
    }
    if (broken_count.value() < 0) {
      DVLOG(1) << "Broken alternative service has negative broken-count.";
      return;
    }
    recently_broken_alternative_services->Put(
        BrokenAlternativeService(alt_service, network_anonymization_key,
                                 use_network_anonymization_key),
        broken_count.value());
    contains_broken_count_or_broken_until = true;
  }

  // Read broken-until and add an entry for |alt_service| in
  // |broken_alternative_service_list|.
  if (broken_alt_svc_entry_dict.Find(kBrokenUntilKey)) {
    const std::string* expiration_string =
        broken_alt_svc_entry_dict.FindString(kBrokenUntilKey);
    int64_t expiration_int64;
    if (!expiration_string ||
        !base::StringToInt64(*expiration_string, &expiration_int64)) {
      DVLOG(1) << "Broken alternative service has malformed broken-until "
               << "string.";
      return;
    }

    time_t expiration_time_t = static_cast<time_t>(expiration_int64);
    // Convert expiration from time_t to Time to TimeTicks
    base::TimeTicks expiration_time_ticks =
        clock_->NowTicks() +
        (base::Time::FromTimeT(expiration_time_t) - base::Time::Now());
    broken_alternative_service_list->emplace_back(
        BrokenAlternativeService(alt_service, network_anonymization_key,
                                 use_network_anonymization_key),
        expiration_time_ticks);
    contains_broken_count_or_broken_until = true;
  }

  if (!contains_broken_count_or_broken_until) {
    DVLOG(1) << "Broken alternative service has neither broken-count nor "
             << "broken-until specified.";
  }
}

void HttpServerPropertiesManager::AddServerData(
    const base::DictValue& server_dict,
    HttpServerProperties::ServerInfoMap* server_info_map,
    bool use_network_anonymization_key) {
  // Get server's scheme/host/pair.
  const std::string* server_str = server_dict.FindString(kServerKey);
  NetworkAnonymizationKey network_anonymization_key;
  // Can't load entry if server name missing, or if the network anonymization
  // key is missing or invalid.
  if (!server_str || !GetNetworkAnonymizationKeyFromDict(
                         server_dict, use_network_anonymization_key,
                         &network_anonymization_key)) {
    return;
  }

  url::SchemeHostPort spdy_server((GURL(*server_str)));
  if (spdy_server.host().empty()) {
    DVLOG(1) << "Malformed http_server_properties for server: " << server_str;
    return;
  }

  HttpServerProperties::ServerInfo server_info;

  server_info.supports_spdy = server_dict.FindBool(kSupportsSpdyKey);

  if (ParseAlternativeServiceInfo(spdy_server, server_dict, &server_info))
    ParseNetworkStats(spdy_server, server_dict, &server_info);

  if (!server_info.empty()) {
    server_info_map->Put(HttpServerProperties::ServerInfoMapKey(
                             std::move(spdy_server), network_anonymization_key,
                             use_network_anonymization_key),
                         std::move(server_info));
  }
}

bool HttpServerPropertiesManager::ParseAlternativeServiceDict(
    const base::DictValue& dict,
    bool host_optional,
    const std::string& parsing_under,
    AlternativeService* alternative_service) {
  // Protocol is mandatory.
  const std::string* protocol_str = dict.FindString(kProtocolKey);
  if (!protocol_str) {
    DVLOG(1) << "Malformed alternative service protocol string under: "
             << parsing_under;
    return false;
  }
  NextProto protocol = NextProtoFromString(*protocol_str);
  if (!IsAlternateProtocolValid(protocol)) {
    DVLOG(1) << "Invalid alternative service protocol string \"" << protocol_str
             << "\" under: " << parsing_under;
    return false;
  }
  alternative_service->protocol = protocol;

  // If host is optional, it defaults to "".
  std::string host = "";
  const std::string* hostp = nullptr;
  if (dict.Find(kHostKey)) {
    hostp = dict.FindString(kHostKey);
    if (!hostp) {
      DVLOG(1) << "Malformed alternative service host string under: "
               << parsing_under;
      return false;
    }
    host = *hostp;
  } else if (!host_optional) {
    DVLOG(1) << "alternative service missing host string under: "
             << parsing_under;
    return false;
  }
  alternative_service->host = host;

  // Port is mandatory.
  std::optional<int> maybe_port = dict.FindInt(kPortKey);
  if (!maybe_port.has_value() || !IsPortValid(maybe_port.value())) {
    DVLOG(1) << "Malformed alternative service port under: " << parsing_under;
    return false;
  }
  alternative_service->port = static_cast<uint32_t>(maybe_port.value());

  return true;
}

bool HttpServerPropertiesManager::ParseAlternativeServiceInfoDictOfServer(
    const base::DictValue& dict,
    const std::string& server_str,
    AlternativeServiceInfo* alternative_service_info) {
  AlternativeService alternative_service;
  if (!ParseAlternativeServiceDict(dict, true, "server " + server_str,
                                   &alternative_service)) {
    return false;
  }
  alternative_service_info->set_alternative_service(alternative_service);

  // Expiration is optional, defaults to one day.
  if (!dict.Find(kExpirationKey)) {
    alternative_service_info->set_expiration(base::Time::Now() + base::Days(1));
  } else {
    const std::string* expiration_string = dict.FindString(kExpirationKey);
    if (expiration_string) {
      int64_t expiration_int64 = 0;
      if (!base::StringToInt64(*expiration_string, &expiration_int64)) {
        DVLOG(1) << "Malformed alternative service expiration for server: "
                 << server_str;
        return false;
      }
      alternative_service_info->set_expiration(
          base::Time::FromInternalValue(expiration_int64));
    } else {
      DVLOG(1) << "Malformed alternative service expiration for server: "
               << server_str;
      return false;
    }
  }

  // Advertised versions list is optional.
  return true;
}

bool HttpServerPropertiesManager::ParseAlternativeServiceInfo(
    const url::SchemeHostPort& server,
    const base::DictValue& server_pref_dict,
    HttpServerProperties::ServerInfo* server_info) {
  DCHECK(!server_info->alternative_services.has_value());
  const base::ListValue* alternative_service_list =
      server_pref_dict.FindList(kAlternativeServiceKey);
  if (!alternative_service_list) {
    return true;
  }
  if (server.scheme() != "https") {
    return false;
  }

  AlternativeServiceInfoVector alternative_service_info_vector;
  for (const auto& alternative_service_list_item : *alternative_service_list) {
    if (!alternative_service_list_item.is_dict())
      return false;
    AlternativeServiceInfo alternative_service_info;
    if (!ParseAlternativeServiceInfoDictOfServer(
            alternative_service_list_item.GetDict(), server.Serialize(),
            &alternative_service_info)) {
      return false;
    }
    if (base::Time::Now() < alternative_service_info.expiration()) {
      alternative_service_info_vector.push_back(alternative_service_info);
    }
  }

  if (alternative_service_info_vector.empty()) {
    return false;
  }

  server_info->alternative_services = alternative_service_info_vector;
  return true;
}

void HttpServerPropertiesManager::ParseNetworkStats(
    const url::SchemeHostPort& server,
    const base::DictValue& server_pref_dict,
    HttpServerProperties::ServerInfo* server_info) {
  DCHECK(!server_info->server_network_stats.has_value());
  const base::DictValue* server_network_stats_dict =
      server_pref_dict.FindDict(kNetworkStatsKey);
  if (!server_network_stats_dict) {
    return;
  }
  std::optional<int> maybe_srtt = server_network_stats_dict->FindInt(kSrttKey);
  if (!maybe_srtt.has_value()) {
    DVLOG(1) << "Malformed ServerNetworkStats for server: "
             << server.Serialize();
    return;
  }
  ServerNetworkStats server_network_stats;
  server_network_stats.srtt = base::Microseconds(maybe_srtt.value());
  server_info->server_network_stats = server_network_stats;
}

void HttpServerPropertiesManager::WriteToPrefs(
    const HttpServerProperties::ServerInfoMap& server_info_map,
    const GetCannonicalSuffix& get_canonical_suffix,
    const BrokenAlternativeServiceList& broken_alternative_service_list,
    const RecentlyBrokenAlternativeServices&
        recently_broken_alternative_services,
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If loading prefs hasn't completed, don't call it, since this will overwrite
  // existing prefs.
  on_prefs_loaded_callback_.Reset();

  std::set<std::pair<std::string, NetworkAnonymizationKey>>
      persisted_canonical_suffix_set;
  const base::Time now = base::Time::Now();
  base::DictValue http_server_properties_dict;

  // Convert |server_info_map| to a list Value and add it to
  // |http_server_properties_dict|.
  base::ListValue servers_list;
  for (const auto& [key, server_info] : server_info_map) {
    // If can't convert the NetworkAnonymizationKey to a value, don't save to
    // disk. Generally happens because the key is for a unique origin.
    base::Value network_anonymization_key_value;
    if (!key.network_anonymization_key.ToValue(
            &network_anonymization_key_value)) {
      continue;
    }

    base::DictValue server_dict;

    bool supports_spdy = server_info.supports_spdy.value_or(false);
    if (supports_spdy)
      server_dict.Set(kSupportsSpdyKey, supports_spdy);

    AlternativeServiceInfoVector alternative_services =
        GetAlternativeServiceToPersist(server_info.alternative_services, key,
                                       now, get_canonical_suffix,
                                       &persisted_canonical_suffix_set);
    if (!alternative_services.empty())
      SaveAlternativeServiceToServerPrefs(alternative_services, server_dict);

    if (server_info.server_network_stats) {
      SaveNetworkStatsToServerPrefs(*server_info.server_network_stats,
                                    server_dict);
    }

    // Don't add empty entries. This can happen if, for example, all alternative
    // services are empty, or |supports_spdy| is set to false, and all other
    // fields are not set.
    if (server_dict.empty())
      continue;
    server_dict.Set(kServerKey, key.server.Serialize());
    server_dict.Set(kNetworkAnonymizationKey,
                    std::move(network_anonymization_key_value));
    servers_list.Append(std::move(server_dict));
  }
  // Reverse `servers_list`. The least recently used item will be in the front.
  std::ranges::reverse(servers_list);

  http_server_properties_dict.Set(kServersKey, std::move(servers_list));

  http_server_properties_dict.Set(kVersionKey, kVersionNumber);

  SaveBrokenAlternativeServicesToPrefs(
      broken_alternative_service_list, kMaxBrokenAlternativeServicesToPersist,
      recently_broken_alternative_services, http_server_properties_dict);

  net_log_.AddEvent(NetLogEventType::HTTP_SERVER_PROPERTIES_UPDATE_PREFS,
                    [&] { return http_server_properties_dict.Clone(); });

  pref_delegate_->SetServerProperties(std::move(http_server_properties_dict),
                                      std::move(callback));
}

void HttpServerPropertiesManager::SaveAlternativeServiceToServerPrefs(
    const AlternativeServiceInfoVector& alternative_service_info_vector,
    base::DictValue& server_pref_dict) {
  if (alternative_service_info_vector.empty()) {
    return;
  }
  base::ListValue alternative_service_list;
  for (const AlternativeServiceInfo& alternative_service_info :
       alternative_service_info_vector) {
    const AlternativeService& alternative_service =
        alternative_service_info.alternative_service();
    DCHECK(IsAlternateProtocolValid(alternative_service.protocol));
    base::DictValue alternative_service_dict;
    AddAlternativeServiceFieldsToDictionaryValue(alternative_service,
                                                 alternative_service_dict);
    // JSON cannot store int64_t, so expiration is converted to a string.
    alternative_service_dict.Set(
        kExpirationKey,
        base::NumberToString(
            alternative_service_info.expiration().ToInternalValue()));
    alternative_service_list.Append(std::move(alternative_service_dict));
  }
  if (alternative_service_list.size() == 0)
    return;
  server_pref_dict.Set(kAlternativeServiceKey,
                       std::move(alternative_service_list));
}

void HttpServerPropertiesManager::SaveNetworkStatsToServerPrefs(
    const ServerNetworkStats& server_network_stats,
    base::DictValue& server_pref_dict) {
  base::DictValue server_network_stats_dict;
  // Because JSON doesn't support int64_t, persist int64_t as a string.
  server_network_stats_dict.Set(
      kSrttKey, static_cast<int>(server_network_stats.srtt.InMicroseconds()));
  server_pref_dict.Set(kNetworkStatsKey, std::move(server_network_stats_dict));
}

void HttpServerPropertiesManager::SaveBrokenAlternativeServicesToPrefs(
    const BrokenAlternativeServiceList& broken_alternative_service_list,
    size_t max_broken_alternative_services,
    const RecentlyBrokenAlternativeServices&
        recently_broken_alternative_services,
    base::DictValue& http_server_properties_dict) {
  if (!base::FeatureList::IsEnabled(
          features::kPersistBrokenAlternativeServices)) {
    return;
  }

  if (broken_alternative_service_list.empty() &&
      recently_broken_alternative_services.empty()) {
    return;
  }

  // JSON list will be in LRU order (least-recently-used item is in the front)
  // according to `recently_broken_alternative_services`.
  base::ListValue json_list;

  // Maps recently-broken alternative services to the index where it's stored
  // in |json_list|.
  std::map<BrokenAlternativeService, size_t> json_list_index_map;

  if (!recently_broken_alternative_services.empty()) {
    for (const auto& [broken_alt_service, broken_count] :
         base::Reversed(recently_broken_alternative_services)) {
      base::DictValue entry_dict;
      if (!TryAddBrokenAlternativeServiceFieldsToDictionaryValue(
              broken_alt_service, entry_dict)) {
        continue;
      }
      entry_dict.Set(kBrokenCountKey, broken_count);
      json_list_index_map[broken_alt_service] = json_list.size();
      json_list.Append(std::move(entry_dict));
    }
  }

  if (!broken_alternative_service_list.empty()) {
    // Add expiration time info from |broken_alternative_service_list| to
    // the JSON list.
    size_t count = 0;
    for (auto it = broken_alternative_service_list.begin();
         it != broken_alternative_service_list.end() &&
         count < max_broken_alternative_services;
         ++it, ++count) {
      const BrokenAlternativeService& broken_alt_service = it->first;
      base::TimeTicks expiration_time_ticks = it->second;
      // Convert expiration from TimeTicks to Time to time_t
      time_t expiration_time_t =
          (base::Time::Now() + (expiration_time_ticks - clock_->NowTicks()))
              .ToTimeT();
      int64_t expiration_int64 = static_cast<int64_t>(expiration_time_t);

      auto index_map_it = json_list_index_map.find(broken_alt_service);
      if (index_map_it != json_list_index_map.end()) {
        size_t json_list_index = index_map_it->second;
        base::Value& entry_dict = json_list[json_list_index];
        DCHECK(entry_dict.is_dict());
        DCHECK(!entry_dict.GetDict().Find(kBrokenUntilKey));
        entry_dict.GetDict().Set(kBrokenUntilKey,
                                 base::NumberToString(expiration_int64));
      } else {
        base::DictValue entry_dict;
        if (!TryAddBrokenAlternativeServiceFieldsToDictionaryValue(
                broken_alt_service, entry_dict)) {
          continue;
        }
        entry_dict.Set(kBrokenUntilKey, base::NumberToString(expiration_int64));
        json_list.Append(std::move(entry_dict));
      }
    }
  }

  // This can happen if all the entries are for NetworkAnonymizationKeys for
  // opaque origins, which isn't exactly common, but can theoretically happen.
  if (json_list.empty())
    return;

  http_server_properties_dict.Set(kBrokenAlternativeServicesKey,
                                  std::move(json_list));
}

void HttpServerPropertiesManager::OnHttpServerPropertiesLoaded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If prefs have already been written, nothing to do.
  if (!on_prefs_loaded_callback_)
    return;

  std::unique_ptr<HttpServerProperties::ServerInfoMap> server_info_map;
  std::unique_ptr<BrokenAlternativeServiceList> broken_alternative_service_list;
  std::unique_ptr<RecentlyBrokenAlternativeServices>
      recently_broken_alternative_services;

  ReadPrefs(&server_info_map, &broken_alternative_service_list,
            &recently_broken_alternative_services);

  std::move(on_prefs_loaded_callback_)
      .Run(std::move(server_info_map),
           std::move(broken_alternative_service_list),
           std::move(recently_broken_alternative_services));
}

}  // namespace net
