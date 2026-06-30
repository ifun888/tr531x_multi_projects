# Light Gun SLE Phase 23: Minimalize Gun Local SLE Client Control

## Goal

Reduce the gun-side local SLE client implementation to the minimum control path needed by the target architecture:

- keep broadcast search, connect, reconnect, exchange-info, property discovery, and data send/receive
- remove sample-only low-latency, PHY, MCS, and throughput-test control logic
- avoid keeping extra sample tuning behavior that is unrelated to the required gun <-> dongle business payload path

## Scope

1. Restrict this phase to the gun-side local SLE client implementation under the project overlay.
2. Keep the existing business packet formats and link hello/ack handshake unchanged.
3. Keep dongle-side code unchanged in this phase.
4. Do not modify `sdk_root_dir`.

## Intended Behavior

- Gun still powers on SLE, scans by advertisement content, connects to the dongle, discovers the writable property, and uses the same send path as before.
- On disconnect or handshake failure, the existing search/reconnect path still returns to broadcast search.
- Gun no longer applies sample low-latency receive callbacks, PHY tuning, MCS tuning, or test-suite command hooks that are not required for the target wireless replacement.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase23_gun_client_minimalize.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/of_sle_uart_client_local.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Notes

- Gun-side local SLE client no longer registers sample test-suite commands for runtime UART PHY / format switching.
- Gun-side local SLE client no longer enables sample low-latency RX callbacks or applies sample low-latency mode, PHY parameter tuning, or MCS tuning on connect.
- The remaining client control path is now limited to:
  - SLE power-on / enable
  - broadcast search
  - advertisement-content match
  - connect / reconnect
  - exchange-info
  - property discovery for the write handle
  - normal notification / indication receive callbacks
- Dongle-side code and the business packet path were intentionally left unchanged.

## Verification Results

1. Gun firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `39b5029d19af5eb8adfb132078068fc51bd32ddb076cca07520f9c24e8be19f2`
2. Dongle firmware build
   Command: `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
   Artifact: `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
   SHA256: `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
