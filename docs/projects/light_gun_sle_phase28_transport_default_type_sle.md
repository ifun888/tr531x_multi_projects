# Light Gun SLE Phase 28: Transport Default Type Aligned To SLE

## Goal

Align transport-layer default state with the staged SLE replacement target:

- both gun and dongle should default their transport-type state to SLE
- default bookkeeping should no longer imply USB CDC is the primary business transport
- runtime packet formats, bridge behavior, and handshake flow remain unchanged

## Scope

1. Restrict this phase to default transport-type bookkeeping in project overlay transport helpers.
2. Preserve actual device selection logic for explicit `OF_TRANSPORT_USB_CDC` requests.
3. Preserve runtime bridge behavior, packet formats, and handshake semantics.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- `of_transport_get_type()` reports an SLE-oriented default before any explicit route switch occurs.
- Gun and dongle route helper internal current-type defaults also align to `OF_TRANSPORT_SLE`.
- Explicit USB CDC bridge use remains available where code directly initializes `OF_TRANSPORT_USB_CDC`.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase28_transport_default_type_sle.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/transport_core.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/transport_core.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated both project-side `transport_core.c` files so their static `g_type` default is now `OF_TRANSPORT_SLE`.
- Updated both project-side `svc_transport_router.c` files so the internal `g_cur_type` default also starts from `OF_TRANSPORT_SLE`.
- Left the explicit device selection branch in `of_transport_init()` unchanged, so direct `OF_TRANSPORT_USB_CDC` initialization still works where the PC bridge path explicitly needs it.
- Left packet framing, hello/ack behavior, bridge data paths, and route switch entrypoints unchanged in this phase.

## Verification Results

### Gun Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `15bd61c1f220149a29dafa12c08158f567ed8e6faaa075add806eea88b191df9`

### Dongle Build

- Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

## Outcome

- Gun and dongle transport bookkeeping now defaults to SLE instead of implicitly advertising USB CDC as the primary route.
- This removes another piece of old USB-first semantics without changing the explicit PC bridge path.
