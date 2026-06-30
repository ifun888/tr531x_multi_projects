# Light Gun SLE Phase 21: Gun Background SLE Handshake

## Goal

Allow the gun to keep the SLE business handshake progressing while its active transport route is still USB CDC, so `of_link_is_ready()` can become true and the router can later switch to SLE without deadlocking on the current transport selection.

## Scope

1. Keep the gun-side SLE driver opened in the background even when USB CDC is the active transport route.
2. Let gun-side hello/ack handshake packets use the physical SLE path before the active transport switches to `OF_TRANSPORT_SLE`.
3. Drain and parse gun-side SLE handshake packets while USB CDC remains the active route.
4. Preserve existing dongle-side behavior except for any shared SLE driver/link helper refcounting needed to support the gun change safely.

## Intended Behavior

- Gun can stay on USB CDC while the physical SLE connection comes up and hello/ack completes in the background.
- `of_link_is_ready()` may become true before the gun's active transport route switches to SLE.
- Once business ready is observed, the existing router logic may switch the active transport to SLE.
- HID/serial/telemetry payload forwarding semantics remain unchanged until the route actually switches.

## Assumptions

- This phase focuses on handshake plumbing and background SLE availability on the gun side.
- Packet formats, retry intervals, ready timeout, and search fallback policy remain unchanged unless required for the background handshake path.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase21_gun_background_sle_handshake.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_sle_link.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_sle_link.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/of_link_io.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/of_link_io.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Gun startup now keeps the SLE driver opened in the background from `of_init_peripherals()`, even while USB CDC is still the active transport route.
- Gun runtime now polls raw SLE bytes when the active route is not `OF_TRANSPORT_SLE`, and feeds those bytes through the existing wireless frame parser so hello/ack packets can complete before route switch.
- Shared SLE driver open/close on both gun and dongle is now reference-counted, so the background-opened SLE instance and later route-managed open/close calls can coexist without tearing down the physical link unexpectedly.
- `of_link_is_ready()` now reflects physical SLE readiness plus completed link handshake, instead of depending on the current transport route already being SLE.
- Before the route switches to SLE, only `OF_WPKT_TYPE_LINK_HELLO` and `OF_WPKT_TYPE_LINK_HELLO_ACK` are allowed to bypass the transport router and go out over raw SLE; all other payload classes still wait for the normal route switch.

## Verification Results

1. Gun firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `a2ee9e871c4d069ce2fe5fe862ed1f2b495758bd327ff55626fb62276f30dcdf`
2. Dongle firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
