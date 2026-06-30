# Light Gun SLE Phase 8: Gamepad Mode Bridge

## Goal

Complete the remaining wireless HID business path by wiring PC-driven input mode selection into the SLE path:

- keep existing mouse, keyboard, serial tunnel, and telemetry behavior unchanged
- when PC-side serial commands switch the gun into gamepad mode, send `OF_WPKT_TYPE_HID_GAMEPAD` over SLE
- make the dongle expose and forward the same gamepad payload as a USB HID gamepad report

## Scope

1. Add a shared wireless packet payload definition for the 16-bit gamepad report.
2. Extend gun-side protocol state so `M0x` serial commands are visible to the HID service.
3. Update gun-side HID service:
   - keep mouse/keyboard behavior for non-gamepad input modes
   - build and send a gamepad wireless packet for gamepad input modes
4. Update dongle-side USB HID driver:
   - register a gamepad HID descriptor
   - send received wireless gamepad payloads as USB HID reports
5. Update dongle runtime packet dispatch to consume `OF_WPKT_TYPE_HID_GAMEPAD`.

## Assumptions

- Current TR project has no separate local analog-stick source, so phase 8 maps the gun aiming position into the wireless gamepad report and maps keys into buttons/hat.
- Existing serial tunnel continues to carry MAMEHooker / host control commands; no new transport command is introduced.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase8_gamepad_mode_bridge.md`
- `projects/light_gun_260517/sdk_overlay/custom/include/of_protocol.h`
- `projects/light_gun_260517/sdk_overlay/custom/include/of_wireless_pkt.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_mh.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_usb_hid.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/drivers/drv_usb_hid.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/of_wireless_pkt.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_usb_hid.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517`.
3. Record firmware artifact paths and SHA256.
4. Update this document with implementation notes and results.

## Implementation Result

Completed:

1. Added `of_wpkt_gamepad_payload_t` to both gun and dongle wireless packet headers.
2. Extended gun-side `proto_mh` state with accessors that expose:
   - current `M0x` input mode
   - whether wireless gamepad mode is enabled
   - whether aim data should map to left stick
3. Updated gun-side HID service so that:
   - SLE + `M01` / `M09` now produce `OF_WPKT_TYPE_HID_GAMEPAD`
   - mouse/keyboard wireless behavior remains unchanged for non-gamepad modes
   - gamepad payload carries:
     - aim position mapped to one analog stick
     - dpad keys mapped to HID hat
     - trigger / A / B / middle / start / select / home mapped to HID gamepad buttons
4. Updated dongle USB HID driver so it now registers a third HID report descriptor for gamepad and can send the wireless gamepad payload as a USB HID report.
5. Updated dongle runtime packet dispatch so `OF_WPKT_TYPE_HID_GAMEPAD` is consumed instead of being treated as unsupported.

## Notes

- This phase keeps local direct-USB gun behavior unchanged; the new gamepad path is activated for the SLE wireless route.
- The current TR gun project does not yet provide a second analog-stick source, so the wireless gamepad payload presently maps aim coordinates into one stick, matching the intended bridge role for SLE replacement.

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
  - `1e4207304802890674f23e925141461e5638c541878570820f6e747943136a90`

### Dongle

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
