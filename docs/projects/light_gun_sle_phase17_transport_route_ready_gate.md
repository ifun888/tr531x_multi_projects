# Light Gun SLE Phase 17: Transport Route Ready Gate

## Goal

Align automatic transport route switching with the SLE business-ready state so the system does not switch away from USB CDC merely because the physical SLE link is connected before the hello/ack business handshake has completed.

## Scope

1. Make transport auto-routing use business-ready wireless state instead of raw physical SLE link state.
2. Keep existing USB CDC priority, cooldown, and stability behavior unchanged.
3. Apply the same routing rule on both the gun and dongle sides.

## Intended Behavior

- While SLE is only physically connected and still waiting for business ready, transport auto-routing should not switch to `OF_TRANSPORT_SLE`.
- Once the wireless business link becomes ready and remains stable, routing may switch to SLE using the existing stability/cooldown logic.
- USB CDC fallback behavior remains unchanged.

## Assumptions

- This phase only changes transport route selection behavior.
- Physical SLE connection management, packet formats, and self-test semantics remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase17_transport_route_ready_gate.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated both gun and dongle `svc_transport_router.c` to include `of_link_io.h` and use `of_link_is_ready()` as the SLE route eligibility signal.
- `svc_transport_route_auto()` no longer treats a merely connected SLE physical link as sufficient to switch transport away from USB CDC.
- `svc_transport_route_tick()` now accumulates SLE stability only after the wireless business-ready state is asserted, preserving the existing stability threshold, hold ticks, and cooldown behavior.
- USB CDC priority and all transport switch timing constants remain unchanged in this phase.

## Verification Results

1. Gun build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `a64489a1b37474a7ad746cd02f3699541ffaec10a8a37db2d4888196cbed8673`
2. Dongle build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
