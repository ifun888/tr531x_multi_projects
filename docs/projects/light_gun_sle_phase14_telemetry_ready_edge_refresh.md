# Light Gun SLE Phase 14: Telemetry Ready-Edge Refresh

## Goal

Make gun-side telemetry publish immediately after the SLE business link becomes ready, instead of being delayed by throttling state that was updated during the not-ready window.

## Scope

1. Track the gun's wireless business-ready edge in the runtime telemetry publisher.
2. Avoid consuming the telemetry throttle interval while the wireless link is physically up but business ready is still false.
3. Force a fresh first telemetry frame immediately after the business-ready transition.

## Intended Behavior

- During the hello/ack window after reconnect, telemetry throttling state should not advance as if a real wireless publish already happened.
- When the business link rises to ready, the next runtime tick should be allowed to publish telemetry immediately.
- Direct wired or non-wireless behavior should remain unchanged.

## Assumptions

- This phase only changes gun-side telemetry publishing behavior.
- Packet formats remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase14_telemetry_ready_edge_refresh.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated `of_publish_runtime_telemetry()` in `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`.
- Added a cached previous wireless-ready flag so the publisher can detect the `not ready -> ready` business-link transition.
- While the SLE physical link is up but `of_link_is_ready()` is still false, telemetry publishing now returns before consuming the 5 ms throttle timestamp.
- On the first tick after the business-ready edge, the cached throttle timestamp is cleared so telemetry can publish immediately instead of waiting for the prior not-ready window to expire.
- Direct or non-wireless execution keeps the original throttling behavior.

## Verification Results

1. Built `light_gun_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `007143e3456b7f42f12e3c98d20b95b05dd62fb8ede45e6b12dd59a26580f34e`
2. Built `light_gun_dongle_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
