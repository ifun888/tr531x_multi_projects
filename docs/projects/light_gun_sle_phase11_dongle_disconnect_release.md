# Light Gun SLE Phase 11: Dongle Disconnect HID Release

## Goal

Prevent stale PC-side HID state when the SLE link drops before the gun can deliver release packets.

## Scope

1. Detect on the dongle when the wireless business link transitions from ready to not-ready.
2. Queue a one-shot neutral HID release for all dongle USB HID families after that transition.
3. Keep packet formats and gun-side behavior unchanged.

## Intended Behavior

- If the gun disconnects while mouse buttons are held, the dongle should send a neutral mouse report to the PC.
- If the gun disconnects while keyboard keys are held, the dongle should send a neutral keyboard report to the PC.
- If the gun disconnects while wireless gamepad mode is active, the dongle should send a neutral gamepad report to the PC.
- If USB HID is not ready at the exact loss moment, the dongle should retry the release until the HID interface is ready.

## Assumptions

- This phase only changes dongle-side runtime and USB HID helper logic.
- `sdk_root_dir` remains untouched.
- Releasing all HID families on disconnect is acceptable even if only one family was active.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase11_dongle_disconnect_release.md`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/drivers/drv_usb_hid.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_usb_hid.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Result

Completed:

1. Added a dongle USB HID helper `drv_usb_hid_release_all()` that sends neutral mouse, keyboard, and gamepad reports to the PC.
2. Added dongle runtime edge detection for `of_link_is_ready()` so a ready-to-not-ready transition schedules a one-shot HID release.
3. If the disconnect happens before the USB HID interface is ready, the dongle now keeps the release pending and retries on later runtime ticks until the neutral reports are sent successfully.

## Notes

- This phase intentionally releases all HID families on disconnect instead of trying to infer the last active family.
- Packet definitions and gun-side wireless business behavior remain unchanged.

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
