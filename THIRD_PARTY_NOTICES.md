# Third-party artwork

## dsh-pet Fatfish animation

The Fatfish idle animation under
`components/mosaic_ui/apps/holo_partner/scene/assets/` is sourced from
[PC2005-cloud/dsh-pet](https://github.com/PC2005-cloud/dsh-pet), revision
`38ae8465cd69ba15e8c8a29e7208997a0baa3451`.

The upstream project describes its animation assets and character artwork as
non-commercial material under CC BY-NC-SA 4.0, with attribution and share-alike
requirements. The firmware project must not be distributed commercially with
these files unless separate permission is obtained from the rights holders.

The idle sequence is derived from these original 640x360 alpha WebM assets:

- `dsh-pet/assets/webm/待机呼吸休闲.webm`
- `dsh-pet/assets/webm/东张西望.webm`

Only transparent margins and the duration needed for the board loop are
trimmed; the character artwork is not redrawn. The PNG beside it is the first
frame extracted solely for the static secondary App view. The APNG is an
intermediate device asset generated from the WebM frames, and the device bundle
is a mechanical ESP-GSP compilation of that APNG.
