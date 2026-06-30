# Light Gun SLE Phase 20: Symmetric Link Ready Ack

## Goal

Make SLE business-ready state symmetric on both gun and dongle so neither side enters ready merely by receiving `LINK_HELLO`; both sides should become ready only after receiving `LINK_HELLO_ACK`.

## Scope

1. Update gun-side link handshake state handling in `of_link_io.c`.
2. Update dongle-side link handshake state handling in `of_link_io.c`.
3. Keep packet formats, retry timers, and search fallback behavior unchanged.

## Intended Behavior

- Receiving `OF_WPKT_TYPE_LINK_HELLO` only records peer presence and replies with `OF_WPKT_TYPE_LINK_HELLO_ACK`.
- `of_link_is_ready()` becomes true only after a side receives `OF_WPKT_TYPE_LINK_HELLO_ACK`.
- Dongle no longer starts business traffic earlier than gun during the handshake window.

## Assumptions

- This phase only changes handshake state transition semantics.
- `LINK_HELLO` and `LINK_HELLO_ACK` packet contents remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase20_symmetric_link_ready_ack.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/of_link_io.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/of_link_io.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated gun and dongle `of_link_io.c` so receiving `OF_WPKT_TYPE_LINK_HELLO` only records peer presence and sends `OF_WPKT_TYPE_LINK_HELLO_ACK`.
- Removed the dongle-only special case that previously set `g_link_ready` immediately on `LINK_HELLO`, making business-ready transition depend exclusively on receiving `LINK_HELLO_ACK`.
- Retry interval, ready timeout, fallback-to-search behavior, and wireless packet formats remain unchanged in this phase.

## Verification Results

1. Gun build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `ac911e5e20579e9b75758c74d76d0f0e66bfdabdb4cb0b1fb6dcb498988b905d`
2. Dongle build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
