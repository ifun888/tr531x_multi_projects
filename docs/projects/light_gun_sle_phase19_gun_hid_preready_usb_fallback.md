# Light Gun SLE Phase 19: Gun HID Pre-Ready USB Fallback

## Goal

Keep the gun-side local USB HID path active while SLE is only physically connected and the wireless business handshake is still not ready, so input reporting does not stall during the pre-ready window.

## Scope

1. Adjust gun-side HID routing logic to treat only business-ready SLE as the wireless-active HID path.
2. Allow local USB HID probing and fallback reporting while SLE is connected physically but `of_link_is_ready()` is still false.
3. Keep dongle behavior, wireless packet formats, and HID report contents unchanged.

## Intended Behavior

- When SLE is not business-ready yet, gun HID should continue using the local USB HID path if available.
- Once SLE becomes business-ready, gun HID should switch to the wireless packet path using the existing route-change handling.
- Existing wireless HID report formats, failure thresholds, and route transition release behavior remain unchanged.

## Assumptions

- This phase only changes gun-side HID path selection semantics in the SLE pre-ready window.
- SLE handshake flow, transport router policy, and dongle USB behavior remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase19_gun_hid_preready_usb_fallback.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_usb_hid.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated gun-side `svc_usb_hid.c` so local USB HID probing and fallback report delivery are gated by SLE business-ready state instead of raw physical SLE link presence.
- Removed the early return that previously discarded HID processing while SLE was physically connected but `of_link_is_ready()` was still false.
- Local USB HID link-failure demotion now also uses business-ready state, keeping fallback recovery available throughout the SLE pre-ready window.
- Wireless HID packet formats, route-change release behavior, and dongle-side code remain unchanged in this phase.

## Verification Results

1. Gun build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `ac911e5e20579e9b75758c74d76d0f0e66bfdabdb4cb0b1fb6dcb498988b905d`
2. Dongle build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
