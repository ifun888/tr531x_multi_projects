# Light Gun SLE Phase 22: Gun Route Latches To SLE After Business Ready

## Goal

Align the gun-side transport policy with the target SLE architecture:

- USB CDC may still be used as a temporary local fallback before the SLE business link is ready.
- Once the gun and dongle complete the SLE business handshake, the gun's active transport route must switch to `OF_TRANSPORT_SLE`.
- After that switch, the gun must not bounce back to `OF_TRANSPORT_USB_CDC` just because USB is still physically ready.
- USB fallback should only be reconsidered when the SLE business link is no longer ready.

## Scope

1. Restrict this phase to gun-side transport route policy.
2. Preserve the existing packet formats, hello/ack handshake, and background SLE handshake plumbing from phase 21.
3. Keep dongle-side bridge behavior unchanged.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Gun boot may still start on USB CDC if SLE business ready has not been established yet.
- When `of_link_is_ready()` becomes stable, the gun route switches to `OF_TRANSPORT_SLE`.
- While SLE business ready remains true, USB readiness alone no longer causes the gun route to switch away from SLE.
- If SLE business ready drops, the existing fallback path may return the gun route to USB CDC.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase22_gun_route_latch_sle.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Gun-side `svc_transport_route_auto()` now prefers `of_link_is_ready()` over USB CDC readiness, so boot and recovery logic will select `OF_TRANSPORT_SLE` as soon as the SLE business link is ready.
- Gun-side `svc_transport_route_tick()` now checks stable SLE business readiness before stable USB CDC readiness, which prevents the gun from switching back to `OF_TRANSPORT_USB_CDC` merely because USB remains physically ready after the SLE route becomes valid.
- USB CDC fallback remains intact when the SLE business link is not ready, so the pre-ready local fallback behavior from earlier phases is preserved.
- Dongle-side code and the phase 21 background SLE handshake path were intentionally left unchanged in this phase.

## Verification Results

1. Gun firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `39b5029d19af5eb8adfb132078068fc51bd32ddb076cca07520f9c24e8be19f2`
2. Dongle firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
