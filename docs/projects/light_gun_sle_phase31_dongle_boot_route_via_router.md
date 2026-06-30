# Light Gun SLE Phase 31: Dongle Boot Route Goes Through Router

## Goal

Align dongle boot-time transport bring-up with the fixed-SLE router architecture:

- dongle boot should no longer initialize SLE transport by bypassing the router
- boot-time route bring-up should use `svc_transport_route_init()` directly
- packet formats, USB bridge behavior, and periodic route maintenance must remain unchanged

## Scope

1. Restrict this phase to the dongle project boot entry path.
2. Update the dongle boot callsite to use the project-side transport router API.
3. Preserve existing USB CDC and USB HID initialization flow.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- The dongle overlay entry initializes business transport via `svc_transport_route_init()`.
- Dongle boot no longer reaches into `of_transport_init(OF_TRANSPORT_SLE)` directly from app entry.
- Runtime route retry behavior continues to use `svc_transport_route_tick()` unchanged.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase31_dongle_boot_route_via_router.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_entry.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Updated dongle `of_entry.c` to include `services/svc_transport_router.h`.
- Replaced the direct `of_transport_init(OF_TRANSPORT_SLE)` boot-time call with `svc_transport_route_init()`.
- Left USB CDC open, USB HID initialization, self-test bring-up, and periodic `svc_transport_route_tick()` maintenance paths unchanged.
- This phase only removes the last boot-entry bypass of the project-side fixed-SLE router on the dongle side.

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

Phase 31 makes dongle boot-time business transport bring-up follow the same fixed-SLE router entrypoint as the gun. The project overlays now initialize the SLE route consistently from app entry without changing packet content or USB bridge behavior.
