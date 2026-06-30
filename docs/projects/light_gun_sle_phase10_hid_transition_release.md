# Light Gun SLE Phase 10: HID Transition Release

## Goal

Prevent stale HID state from surviving across transport and input-mode transitions by explicitly releasing the previous report family before switching the gun’s active HID routing.

## Scope

1. Detect transitions between:
   - direct USB HID path and SLE wireless HID path
   - wireless mouse/keyboard mode and wireless gamepad mode
2. When leaving a wireless HID family, emit neutral release reports for the previous family before starting the new one.
3. Reset local HID comparison caches on route-family transitions so the new destination receives the current state immediately.

## Intended Behavior

- Switching `M00`/mouse-keyboard wireless mode to `M01`/`M09` gamepad mode should first release any wireless mouse buttons and keyboard keys.
- Switching wireless gamepad mode back to mouse-keyboard mode should first send a neutral wireless gamepad report.
- Entering a new route family should not suppress the first report just because the local cache still matches the previous destination.

## Assumptions

- This phase only changes gun-side HID state management.
- Wireless packet formats remain unchanged.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase10_hid_transition_release.md`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_usb_hid.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517` to confirm no cross-project regression.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Result

Completed:

1. Added route-family tracking in gun HID service for:
   - whether the current HID destination is wireless or direct USB
   - whether the current wireless HID family is mouse/keyboard or gamepad
2. When leaving a wireless HID family:
   - previous wireless gamepad state is now released with a neutral gamepad report
   - previous wireless mouse buttons and keyboard keys are now released with neutral reports
3. After a route-family transition, local HID comparison caches are reset so the new destination receives the next current-state report instead of being suppressed by stale cache matches.

## Notes

- This phase specifically targets route/mode transition correctness; it does not change any packet formats or steady-state mappings.
- Leaving wireless because of an actual link loss still cannot guarantee delivery of a final release packet, but the local caches are reset so the next available route starts cleanly.

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
