# Light Gun SLE Phase 18: Selftest Business Ready Gate

## Goal

Align the SLE self-test result with the wireless business-ready state so diagnostics no longer report SLE as ready merely because the physical link is connected before the hello/ack handshake completes.

## Scope

1. Make the gun self-test use `of_link_is_ready()` for `OF_ST_SLE`.
2. Make the dongle self-test use `of_link_is_ready()` for `OF_ST_SLE`.
3. Keep all other self-test items and severity levels unchanged.

## Intended Behavior

- While SLE is only physically connected and still waiting for business ready, `OF_ST_SLE` remains `WARN`.
- Once the wireless business link is ready, `OF_ST_SLE` reports `PASS`.
- USB CDC and other self-test checks remain unchanged.

## Assumptions

- This phase only updates self-test reporting semantics.
- Wireless transport, handshake flow, and packet payloads remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase18_selftest_business_ready_gate.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/sm/sm_selftest.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/sm/sm_selftest.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated both gun and dongle `sm_selftest.c` to include `of_link_io.h` and use `of_link_is_ready()` for the `OF_ST_SLE` result.
- Self-test now reports SLE `WARN` until the hello/ack business handshake completes, even if the physical SLE transport is already connected.
- All other self-test checks, result severities, and initialization flow remain unchanged in this phase.

## Verification Results

1. Gun build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `30237f273122532d168112efd529854e0d86ac8fed5437caac281b10eb427bbe`
2. Dongle build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
