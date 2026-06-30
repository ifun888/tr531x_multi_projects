# Light Gun SLE Phase 25: Dongle Transport Route Fixed To SLE

## Goal

Align dongle-side transport routing semantics with the target architecture:

- dongle wireless business transport remains SLE
- dongle USB CDC stays as the PC-facing bridge only
- remove the residual router policy that still treats USB CDC as a competing active transport

## Scope

1. Restrict this phase to dongle-side transport route policy.
2. Preserve dongle-side USB bridge behavior in `of_runtime.c`.
3. Preserve packet formats, handshake behavior, and gun-side code.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Any dongle-side route helper still present in the overlay now resolves to `OF_TRANSPORT_SLE`.
- Dongle-side transport route ticking no longer considers USB CDC readiness as a reason to switch the active transport away from SLE.
- Dongle USB CDC remains opened and used by the explicit bridge path, not by transport-router competition.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase25_dongle_route_fixed_sle.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Simplified `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c` so the dongle-side router no longer evaluates USB CDC and SLE as competing transports.
- Kept the router entrypoints (`svc_transport_route_init`, `svc_transport_route_auto`, `svc_transport_route_tick`) but made each resolve directly to `OF_TRANSPORT_SLE`.
- Retained minimal transport re-init protection inside `svc_route_switch()` so repeated route ticks do not churn the transport stack when SLE is already active.
- Left `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c` untouched, so USB CDC still serves only as the explicit PC bridge path.
- Left packet format, hello/ack flow, and gun-side logic unchanged in this phase.

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

- Dongle transport-route policy is now fixed to SLE and no longer encodes USB-vs-SLE arbitration.
- Dongle USB remains available only for the PC bridge path, which matches the staged SLE replacement target for phase 25.
