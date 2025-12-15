# Mach Port Flooder

DoS-only PoC that inflates launchd’s footprint with OOL port arrays to force an `initproc exited` panic. Not a jailbreak primitive.

## Tested
- iPhone 15 Plus – iOS 26.2 (23C55) ✅
- iPad – iPadOS 18.0 ✅

## Status
- Works: ✅ (2025-12-14)
- Unpatched on iOS 26.2 (23C55)

## Notes
- Expected panic string: `initproc exited -- exit reason namespace 1 subcode 0x7`.
- This will force a full device panic/reboot; use entirely at your own risk and never on a primary device.
- Credits: Original by Jian Lee (@speedyfriend433); stability tweaks by yousef (yousef_dev921).
