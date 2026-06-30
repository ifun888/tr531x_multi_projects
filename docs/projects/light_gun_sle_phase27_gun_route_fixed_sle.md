# Light Gun SLE Phase 27: Gun Transport Route Fixed To SLE

## Goal

Align gun-side transport routing semantics with the staged SLE replacement target:

- gun wireless business transport remains SLE
- gun no longer keeps USB CDC as an alternative active transport route
- remove the residual gun-side router policy that still treats USB CDC as a competing or fallback transport

## Scope

1. Restrict this phase to gun-side transport route policy and boot-time route selection.
2. Preserve packet formats, hello/ack handshake behavior, and wireless payload semantics.
3. Preserve dongle-side code in this phase.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Gun-side route helpers resolve to `OF_TRANSPORT_SLE`.
- Gun boot no longer falls back to `OF_TRANSPORT_USB_CDC` when initializing the active transport.
- Gun continues to use the existing SLE handshake and wireless business packet flow.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase27_gun_route_fixed_sle.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Simplified `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c` so the gun-side router no longer evaluates USB CDC and SLE as competing transports.
- Kept the route helper entrypoints (`svc_transport_route_init`, `svc_transport_route_auto`, `svc_transport_route_tick`) but made each resolve directly to `OF_TRANSPORT_SLE`.
- Retained minimal transport re-init protection inside `svc_route_switch()` so repeated route ticks do not churn the transport stack when SLE is already active.
- Updated `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c` so boot no longer falls back to `OF_TRANSPORT_USB_CDC`; it now just starts the project route policy, which resolves to SLE.
- Left packet format, hello/ack flow, wireless framing, and dongle-side code unchanged in this phase.

## Verification Results

### Gun Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `abad5d2850117f2d57b2917c4577a85dbbd5336e9f277d494799e9afcf3da7fc`

### Dongle Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

## Outcome

- Gun transport-route policy is now fixed to SLE and no longer encodes USB-vs-SLE arbitration or boot-time fallback.
- Both sides now treat SLE as the only active business transport route, which is closer to the target staged replacement architecture.
