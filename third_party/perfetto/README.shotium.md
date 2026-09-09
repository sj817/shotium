# perfetto source slice

Based on upstream fd896b1ad88899807bb128b7255deb5199dde54f. Maintained directly in the shotium repository.
Existing local modifications have been retained in source; no patch replay
is required. This initial source selection retains implementation, headers,
GN metadata, generators, licenses and all tracked inputs found in historical
platform graphs. It is not a proof that all retained source is required.
This baseline passed all six platform builds for shotium 0.6.0 at
07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f. See docs/upstream-sync-state.json
and docs/upstream-sync-0.6.0-validation.md for scope and verification limits.
