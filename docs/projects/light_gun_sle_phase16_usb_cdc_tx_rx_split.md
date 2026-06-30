# Light Gun SLE Phase 16: USB CDC Tx/Rx Split

## Goal

Prevent USB CDC write failures from feeding outbound serial data back into the inbound read path by separating RX and TX buffering and retrying deferred TX flushes explicitly.

## Scope

1. Split USB CDC fallback buffering into independent RX and TX queues.
2. Keep outbound bytes queued for later host delivery instead of exposing them through the inbound read path.
3. Retry deferred TX flushes during normal driver activity without changing the higher-level serial protocol.
4. Apply the same fix to both the gun and dongle local USB CDC drivers.

## Intended Behavior

- Incoming USB CDC bytes from the host should only appear in the RX path.
- Outgoing USB CDC bytes that cannot be written immediately should stay in a TX queue and be retried later.
- A temporary USB CDC write failure must not create protocol loopback by making outbound bytes readable as if they came from the PC.
- Existing higher-level transport routing should continue to use the same driver API.

## Assumptions

- This phase only changes local USB CDC buffering semantics.
- Wireless packet formats and USB command payloads remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase16_usb_cdc_tx_rx_split.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated both local USB CDC drivers:
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`
  - `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`
- Replaced the single shared fallback ring buffer with independent RX and TX queues.
- `drv_usb_cdc_push_rx()` now only feeds the RX queue, so inbound host data remains isolated from deferred outbound traffic.
- `usb_write()` now queues failed or partially written outbound bytes in the TX queue instead of exposing them through `usb_read()`.
- Added deferred TX flushing during `read`, `write`, and `resume` activity so queued outbound USB CDC bytes are retried without changing the higher-level driver API.
- Preserved existing project-specific open behavior:
  - gun keeps prior ready state across reopen
  - dongle still starts with `ready = 0` on open

## Verification Results

1. Built `light_gun_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `29df1315af818e3918589768b166bd65531ae54ef719fbf62ef88e0fdcbf3253`
2. Built `light_gun_dongle_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
