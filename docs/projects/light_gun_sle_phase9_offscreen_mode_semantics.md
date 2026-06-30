# Light Gun SLE Phase 9: Offscreen Mode Semantics

## Goal

Make host-driven offscreen mode selection (`M1x`) take effect in the gun HID business path so the SLE replacement follows the original serial-mode semantics instead of using a fixed offscreen trigger mapping.

## Scope

1. Expose gun-side MAMEHooker offscreen mode state from `proto_mh`.
2. Update gun-side HID routing so offscreen trigger behavior depends on `M1x`.
3. Keep wireless payload types unchanged:
   - mouse mode still sends mouse packets
   - gamepad mode still sends gamepad packets
4. Restrict this phase to offscreen trigger semantics only.

## Intended Behavior

- `M1x0`, `M1x1`, `M1x3`
  - treat offscreen special-shot behavior as disabled
  - trigger only acts as the normal trigger when aim is valid
- `M1x2`
  - enable offscreen button mode
  - when aim is invalid/offscreen, trigger should emit the same logical output as Button A

## Assumptions

- Current TR project has no explicit reload button or low-button remap layer, so “same logical output as Button A” maps to the project’s existing Button A output path.
- This phase does not add pedal-source hardware support; `M2x` remains out of scope.
- `sdk_root_dir` remains untouched.

## Files Expected To Change

- `docs/projects/light_gun_sle_phase9_offscreen_mode_semantics.md`
- `projects/light_gun_260517/sdk_overlay/custom/include/of_protocol.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_mh.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_usb_hid.c`

## Verification Plan

1. Build `light_gun_260517`.
2. Build `light_gun_dongle_260517` to confirm no cross-project regression.
3. Record artifact paths and SHA256.
4. Update this document with implementation notes and verification results.

## Implementation Result

Completed:

1. Added gun-side protocol accessors for:
   - current offscreen mode
   - whether offscreen button mode is enabled
2. Updated mouse-path trigger handling:
   - valid aim keeps the normal trigger output
   - invalid aim only emits the Button A-equivalent output when `M1x2` is active
   - `M1x0`, `M1x1`, `M1x3` no longer force the old hard-coded offscreen trigger substitution
3. Updated wireless gamepad-path trigger handling with the same semantics:
   - valid aim keeps trigger-to-gamepad-trigger mapping
   - invalid aim under `M1x2` remaps trigger to the Button A-equivalent gamepad button
   - invalid aim under the other offscreen modes suppresses the offscreen trigger override

## Notes

- This phase intentionally leaves pedal mode (`M2x`) untouched because the current TR project still lacks a dedicated pedal input source.
- Wireless packet formats remain unchanged; the phase only changes when the trigger contributes to the existing mouse/gamepad payloads.

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
  - `138c8150b69f0468d79f1d122558d72c6e38d37f66c0a964732a888d0b172b98`

### Dongle

- Command:
  - `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
- Result:
  - success
- Artifact:
  - `projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
- SHA256:
  - `673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
