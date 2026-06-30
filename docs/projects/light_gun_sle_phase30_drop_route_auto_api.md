# Light Gun SLE Phase 30: Drop Legacy Route Auto API

## Goal

Remove the last project-side transport-router API entrypoint that still implies dynamic route arbitration:

- `svc_transport_route_auto()` should no longer exist as a public interface
- route initialization should use the explicit fixed-SLE initializer directly
- runtime packet formats, bridge behavior, and SLE handshake flow must remain unchanged

## Scope

1. Restrict this phase to project-side transport-router API cleanup.
2. Update both gun and dongle router headers and implementations.
3. Update any remaining callsites that still use the legacy auto-route entrypoint.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Boot-time route bring-up uses `svc_transport_route_init()` directly.
- No public `svc_transport_route_auto()` API remains in either project overlay.
- Periodic route maintenance via `svc_transport_route_tick()` remains unchanged in this phase.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase30_drop_route_auto_api.md`
- `projects/light_gun_260517/sdk_overlay/custom/include/services/svc_transport_router.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/services/svc_transport_router.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Removed `svc_transport_route_auto()` from both gun and dongle transport-router public headers.
- Removed the matching dead `svc_transport_route_auto()` implementation from both router source files.
- Updated the gun boot entry to include `svc_transport_router.h` directly and call `svc_transport_route_init()` instead of relying on a forward-declared auto-route API.
- Left `svc_transport_route_tick()` intact for periodic fixed-SLE route maintenance and retry semantics.
- Kept packet framing, SLE handshake, and USB bridge behavior unchanged.

## Verification Results

### Gun

- Build command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `8a008fe58955ea5ed5d005a958f559e2cb2a3e2a99bff8d7820d11fbaa724571`

### Dongle

- Build command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

## Outcome

Phase 30 removes another obsolete route-arbitration API surface from the project overlays. Boot-time route bring-up now uses the explicit fixed-SLE initializer directly, which better matches the current single-route architecture without changing runtime business behavior.
