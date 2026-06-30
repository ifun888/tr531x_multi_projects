# Light Gun SLE Phase 12: MH Replay After Reconnect

## Goal

Restore host-driven MAMEHooker volatile control state on the gun after an SLE reconnect, without requiring the PC to manually resend the last mode and feedback commands.

## Scope

1. On the dongle, observe USB CDC downlink traffic and shadow-cache the latest recognized MH control commands.
2. When the SLE business link transitions back to ready, replay the cached volatile MH control commands to the gun in a stable order.
3. Keep wireless packet types and gun-side MH protocol format unchanged.

## Intended Behavior

- If the PC previously sent `M0=1`, `M1=2`, `XI3`, or similar MH mode/config commands, those settings should be resent to the gun after reconnect.
- If the PC previously sent feedback commands like `F1xx`, `F2xx`, or burst/display commands, those commands should be resent after reconnect.
- If the PC ended the session with `E`, the replay cache should be cleared instead of restoring stale session state on the next reconnect.
- Query-only commands such as `PING`, `MODE?`, and `STATUS?` should not be replayed.

## Assumptions

- This phase only targets MH ASCII downlink control consistency; docked binary configuration persistence remains out of scope.
- USB CDC traffic still forwards live bytes exactly as before.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase12_mh_replay_after_reconnect.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Result

Completed:

1. Added a lightweight dongle-side MH shadow parser for the USB CDC downlink stream.
2. The dongle now caches the latest recognized volatile MH control commands, including:
   - session start
   - `MODE=<n>`
   - `M0/M1/M2/M3/M6/M8`
   - `XI/XR`
   - `R0` to `R4`
   - `F0` to `F4` and `FD`
3. Query-only commands such as `PING`, `MODE?`, and `STATUS?` are consumed by the shadow parser only for cache classification and are not replayed.
4. When the SLE business link rises from not-ready to ready, the dongle now schedules a one-shot replay of cached MH control state back to the gun in a deterministic order.
5. Receiving `E` clears the replay cache so the next reconnect does not restore stale session state.

## Notes

- This phase intentionally keeps the live USB CDC forwarding path unchanged while the link is ready; the shadow parser only mirrors recognized MH control state for reconnect recovery.
- This phase does not attempt to persist or replay docked binary configuration traffic.
- Cached replay is best-effort and idempotent by design: on a transient replay send failure, the dongle keeps the replay pending and retries on later runtime ticks while the link is ready.

## Verification Result

Builds were executed serially to avoid `build_project.sh` cross-project contamination.

### Gun

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `c425ebaef14f7883168bdc49069b062134831d4883002281ce825c96ad0884a3`

### Dongle

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
