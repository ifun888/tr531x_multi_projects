# Light Gun SLE Phase 24: Gun Selftest Drops USB CDC Warning

## Goal

Align gun-side selftest semantics with the target SLE architecture:

- keep the existing selftest layout and result slots compatible
- stop treating gun-side USB CDC readiness as an expected warning condition
- keep SLE business readiness as the relevant wireless transport selftest signal

## Scope

1. Restrict this phase to gun-side selftest evaluation.
2. Preserve existing selftest enum layout and protocol-visible slot ordering.
3. Keep dongle-side selftest behavior unchanged.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Gun selftest still reports the existing result array and bitmap shape.
- `OF_ST_USB` remains present for compatibility, but gun selftest no longer marks it `WARN` merely because USB CDC is not ready.
- `OF_ST_SLE` continues to reflect the gun-side SLE business-ready state.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase24_gun_selftest_drop_usb_cdc_warn.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/sm/sm_selftest.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Gun-side `sm_selftest.c` no longer includes `drv_usb_cdc.h` or marks `OF_ST_USB` as `WARN` when USB CDC is not ready.
- The `OF_ST_USB` slot and bitmap layout were intentionally kept unchanged, so protocol-visible selftest ordering remains compatible.
- Gun-side `OF_ST_SLE` continues to represent the wireless transport health signal through `of_link_is_ready()`.
- Dongle-side selftest behavior was intentionally left unchanged in this phase.

## Verification Results

1. Gun firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `ec9a991aab87c2fc969f69bb510f0152cd97949a0c01aec921f99329303cee9a`
2. Dongle firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
