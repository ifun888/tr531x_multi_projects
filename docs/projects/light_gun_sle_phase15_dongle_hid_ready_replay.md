# Light Gun SLE Phase 15: Dongle HID Ready Replay

## Goal

Make the dongle's USB HID bridge wait for actual host readiness and replay the latest wireless HID state once the USB HID path becomes available, so early wireless mouse/keyboard/gamepad packets are not silently lost.

## Scope

1. Stop treating dongle USB HID as ready immediately after initialization.
2. Probe USB HID readiness at runtime on the dongle side.
3. Cache the latest incoming wireless HID payload per report family and replay it after USB HID becomes ready again.
4. Keep protocol formats and wireless packet contents unchanged.

## Intended Behavior

- If the dongle USB HID stack is initialized before the host has finished enumerating the interface, incoming wireless HID packets should be cached instead of discarded.
- Once USB HID probing succeeds, the dongle should flush the latest cached HID state to the host.
- If USB HID output fails later, the dongle should fall back to not-ready, retry probing, and replay the latest cached state after recovery.
- Wireless serial tunnel and telemetry bridging should remain unchanged.

## Assumptions

- This phase only changes dongle-side USB HID readiness and replay behavior.
- The gun continues to send the same HID payloads and packet types as before.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase15_dongle_hid_ready_replay.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_entry.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_entry.c` so dongle USB HID initialization no longer marks the HID path ready immediately after startup.
- Added a dongle-side HID replay context in `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c` to cache the latest wireless mouse, keyboard, and gamepad payloads independently.
- Added a runtime USB HID readiness probe loop that calls `drv_usb_hid_probe_ready()` until the host-side HID path is actually usable.
- On the USB HID ready rising edge, the dongle now schedules replay of every cached HID family so the latest wireless state is flushed to the host.
- If a HID send fails after readiness was established, the dongle now drops back to not-ready, keeps the cached state pending, and retries after the next successful host-ready probe.
- On wireless link loss, the cached HID replay state is cleared so stale pre-disconnect reports are not replayed into the next session.

## Verification Results

1. Built `light_gun_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `007143e3456b7f42f12e3c98d20b95b05dd62fb8ede45e6b12dd59a26580f34e`
2. Built `light_gun_dongle_260517` with:
   `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
