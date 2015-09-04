# Ära unusta configit üle võtta!

_patches=(
00-dwm-6.0-buildflags.diff
01-dwm-6.0-xft.diff
02-dwm-6.0-pertag2.diff
03-dwm-6.0-systray.diff
04-dwm-6.0-statuscolors.diff
05-dwm-6.0-occupiedcol.diff
06-dwm-6.0-gaplessgrid.diff
07-dwm-6.0-XKeycodeToKeysym_fix.diff    # Kuni siiani (k.a polnud bar f-ed up)
08-dwm-6.0-enternotify.diff
09-dwm-6.0-reload.diff
10-dwm-6.0-cycle.diff
11-dwm-6.0-pangofix.diff
12-dwm-6.0-moveresize.diff
13-dwm-6.0-push.diff
14-dwm-6.0-bstack.diff
15-dwm-6.0-centred-floating.diff
16-dwm-6.0-save_floats.diff
17-dwm-6.0-uselessgaps.diff
18-dwm-6.0-bartrans.diff
19-dwm-6.0-bgtag.diff
)

for PATCH in "${_patches[@]}"; do
    echo "=> $PATCH"
    patch -p1 < $PATCH || { echo "Error! press any key to continue with patching"; read dummy; }
done

echo "DONE woooooo"
exit 0
