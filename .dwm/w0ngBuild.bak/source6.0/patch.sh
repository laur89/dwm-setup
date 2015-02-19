# Ära unusta configit üle võtta!

# are we in src dir?

patchDir=".."
files( \
    "bstack.c" \
    "push.c" \
     )

for i in ${files[@]}; do
    cp ../$i ./
done

_patches=(
01-statuscolours.diff
02-monoclecount.diff
03-noborder.diff
04-centredfloating.diff
05-scratchpad.diff
06-attachaside.diff
07-nopaddedbar.diff
08-selfrestart.diff
)

for PATCH in "${_patches[@]}"; do
    echo "=> $PATCH"
    patch -p1 < $patchDir/$PATCH || { echo "Error! press any key to continue with patching"; read dummy; }
done

echo "DONE woooooo"
exit 0
