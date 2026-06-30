# Light Gun SLE Phase 26: Dongle Selftest Drops USB CDC Warning

## Goal

Align dongle-side self-test semantics with the staged SLE replacement target:

- dongle wireless readiness should be represented by the SLE business link state
- dongle USB CDC should remain a bridge implementation detail
- diagnostics should no longer warn merely because dongle USB CDC is not ready

## Scope

1. Restrict this phase to dongle-side self-test evaluation.
2. Preserve dongle runtime bridge behavior and USB data path.
3. Preserve packet formats, handshake behavior, and gun-side code.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Dongle self-test keeps the `OF_ST_USB` slot for compatibility, but it no longer degrades to `WARN` when USB CDC is absent.
- Dongle self-test continues to report `OF_ST_SLE` from `of_link_is_ready()`, so wireless business readiness still shows accurately.
- Dongle runtime and USB bridge logic remain unchanged.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase26_dongle_selftest_drop_usb_cdc_warn.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/sm/sm_selftest.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Removed the dongle self-test dependency on `drv_usb_cdc_is_ready()` in `projects/light_gun_dongle_260517/sdk_overlay/custom/src/sm/sm_selftest.c`.
- Kept the `OF_ST_USB` slot intact for compatibility with existing self-test bitmap/index consumers.
- Left `OF_ST_SLE` evaluation unchanged, so dongle wireless readiness still follows `of_link_is_ready()` and therefore the SLE business-ready state.
- Left dongle runtime bridge logic and all USB CDC data-path behavior unchanged in this phase.

## Verification Results

### Gun Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `ec9a991aab87c2fc969f69bb510f0152cd97949a0c01aec921f99329303cee9a`

### Dongle Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

## Outcome

- Dongle diagnostics no longer report a USB warning merely because the PC-facing CDC bridge is absent.
- Dongle self-test now better matches the staged SLE replacement architecture where wireless business readiness is represented by `OF_ST_SLE`.
