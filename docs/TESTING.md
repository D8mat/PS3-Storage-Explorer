# Testing 0.1.7 on PS3 over FTP

Install the PKG as described in the README. Start with a small known fixture before scanning your library.

1. Extract `dist/USB-TEST.zip` on the PC. Despite its historical filename, it also works over FTP.
2. Upload the entire `STORAGE_TEST` folder to `/dev_hdd0/STORAGE_TEST`. Ensure the empty `C_EMPTY` directory is created too.
3. Add `/dev_hdd0/STORAGE_TEST` to `/dev_hdd0/game/STOR00001/USRDIR/paths.txt`, preserving any existing custom entries. Use UTF-8 without BOM.
4. Launch the app. Check READY and version 0.1.7; press Start to verify return to XMB, then launch again.
5. Press Triangle, refresh with Select, highlight the custom test path and press X with no other paths marked.

| Item | Bytes | Files |
| --- | ---: | ---: |
| B_BIG | 3,145,728 | 2 |
| A_SMALL | 1,048,576 | 1 |
| C_EMPTY | 0 | 0 |

Total: **4,194,304 bytes**. Read exact bytes in the selected-item details; GiB values are rounded. Square should reverse the order. Open B_BIG: `two_mib.bin` is 2,097,152 bytes and NESTED is 1,048,576 bytes. Circle returns to the parent.

Then test your library:

- Mark two existing game paths with Square, press X and verify a combined size-sorted list. Check the full path for similarly named entries.
- Use L1/R1 and Up/Down through more than one page.
- Select rescans. Circle cancels an active scan; partial results may appear.
- Start returns to XMB. Cancellation/exit can be delayed by blocking filesystem I/O.

If the console freezes, record the last visible path/status, selected roots, firmware, homebrew environment and video resolution. Try Start or the PS menu; if unresponsive, use the console power button and allow any storage check to complete on restart. Do not repeatedly retry a freezing scan without investigating it.

Earlier scanner behavior was user-tested on CECHL04/4.92/1080p. Full 0.1.7 console testing remains necessary; desktop tests do not verify hardware compatibility.
