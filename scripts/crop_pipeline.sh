#!/bin/bash

# software paths
WIPSHOME=${HOME}/git/wips
TIFFO=${WIPSHOME}/src/tiffo
CROPSCRIPT=${WIPSHOME}/scripts/crop_random.sh

# input options
MULTICHANNEL=$1
COLORIZED=$2
PALETTE=$3
CHANNELS=$4

requested_channels=(${CHANNELS//,/ })

# hard coded options
SCALE=0.325
image_width=1846 # 1846 = 600 um at 0.325
image_height=1846

# Check if COLORIZED file already exists
if [ -f "$COLORIZED" ]; then
    echo " - $COLORIZED already exists. Delete first to re-run colorize."
else
    # run the colorize tiffo
    echo "###### RUNNING COLORIZE (tiffo.cpp) ######"
    ${TIFFO} colorize ${MULTICHANNEL} ${COLORIZED} -C ${CHANNELS} -p $PALETTE
fi

# create a folder to put crops in
color_base="${COLORIZED%.*}"
mkdir -p ${color_base}
random_number=$((RANDOM % 90000 + 10000))
output=${color_base}/${color_base}_crop${random_number}.jpg

# run the crop thing
echo "###### RUNNING CROP (crop_random.sh) -- to ${output} ######"
${CROPSCRIPT} ${COLORIZED} $image_width $image_height $PALETTE $CHANNELS ${output}
