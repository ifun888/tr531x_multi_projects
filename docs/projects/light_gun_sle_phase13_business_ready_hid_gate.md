# Light Gun SLE Phase 13: Business-Ready HID Gate

## Goal

Ensure the gun only commits wireless HID state after the SLE business link is actually ready, so the first effective HID state after reconnect is not lost or locally suppressed during the hello/ack window.

## Scope

1. Separate "physical SLE connected" from "business link ready" in the gun HID service.
2. Prevent wireless HID cache updates while the physical link is up but `of_link_is_ready()` is still false.
3. Force a fresh first HID report after the business-ready transition by resetting the relevant local comparison state.

## Intended Behavior

- During the hello/ack window after SLE reconnect, the gun should not treat wireless HID packets as already delivered.
- Once the business link becomes ready, the next current HID state should be sent even if buttons/mode/aim state did not change since the physical reconnect.
- The gun should not incorrectly fall back to direct USB HID probing while SLE is physically connected but still waiting for business ready.

## Assumptions

- This phase focuses on gun-side HID up-link behavior only.
- Packet formats remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase13_business_ready_hid_gate.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_usb_hid.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Result

Completed:

1. Split gun HID routing into:
   - physical SLE link up
   - business-ready wireless HID path gated by `of_link_is_ready()`
2. The gun no longer treats the hello/ack window as an already-usable wireless HID route.
3. On wireless route-family transitions, the gun now resets both:
   - cached HID report comparisons
   - relative-motion history used for mouse delta generation
4. While SLE is physically connected but business ready is still false:
   - direct USB HID probing stays suppressed
   - wireless HID report state is not committed locally
   - relative motion history is cleared so the first ready-state mouse frame starts cleanly
5. Once the business link becomes ready, the next current HID state is forced through the normal send path instead of being suppressed by stale pre-ready cache state.

## Notes

- This phase only changes gun-side HID routing behavior; packet formats remain unchanged.
- The direct USB fallback still only activates when the physical SLE link is actually down.
- This phase does not attempt to reconstruct relative mouse motion that occurred during the hello/ack window; it ensures the first post-ready state starts from a clean baseline instead of being falsely considered already delivered.

## Verification Result

Builds were executed serially to avoid `build_project.sh` cross-project contamination.

### Gun

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `c1df770debac71316c36b158ab2481ba780fc7cffa7b6dc4e4ad7b08ffced99f`

### Dongle

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
