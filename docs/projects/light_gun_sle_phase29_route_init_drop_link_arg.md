# Light Gun SLE Phase 29: Route Init Drops Legacy Link Argument

## Goal

Remove the residual legacy link-selection API shape from project-side transport routing:

- transport route init should no longer pretend to choose between link families
- both gun and dongle already use fixed SLE route policy, so the old `link` argument is dead
- keep runtime behavior unchanged while tightening the interface

## Scope

1. Restrict this phase to project-side transport-router interface cleanup.
2. Update both gun and dongle transport-router headers and implementations.
3. Preserve route behavior, packet formats, and bridge/runtime logic.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- `svc_transport_route_init()` becomes a no-argument initializer.
- The interface no longer exposes the obsolete “select route by `of_link_type_t`” model.
- Runtime route policy remains fixed to SLE on both sides.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase29_route_init_drop_link_arg.md`
- `projects/light_gun_260517/sdk_overlay/custom/include/services/svc_transport_router.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_transport_router.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/services/svc_transport_router.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_transport_router.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Removed the dead `of_link_type_t` parameter from `svc_transport_route_init()` in both gun and dongle router headers.
- Removed the matching unused argument handling in both router implementations.
- Kept runtime behavior unchanged: route initialization still switches directly to `OF_TRANSPORT_SLE`.
- The router interface now matches the post-refactor architecture where route selection is no longer negotiated by link family.

## Verification Results

### Gun

- Build command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `3f5b3a1fdaaf737eff51d5eeb9c322d9c9ceb31fcbc5e10fcea7e308c57faf88`

### Dongle

- Build command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

## Outcome

Phase 29 removes the last project-side API shape that implied dynamic link-family route selection. Both light gun and dongle now expose a route-init interface consistent with the fixed SLE transport design, with no packet or runtime behavior changes.
