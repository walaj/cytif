#!/bin/bash

TIF=$1

# Specify the dimensions of the sub-image you want
image_width=$2
image_height=$3

# make a tmp file name to store intermediate output
# Generate a random temporary file name in the /tmp directory
tmp_file=$(mktemp /tmp/wips.XXXXXX)

# parameters
JPEG_QUALITY=85
FONT="Arial"
FONTSIZE=60

# place images in one spot
NEWDIR="cropped"
mkdir -p ${NEWDIR}
random_number=$((RANDOM % 90000 + 10000))
OUTJPG=${NEWDIR}/crop${random_number}.jpg
echo "writing to $OUTJPG"

# Get the dimensions of the original image
originalWidth=$(magick identify -format "%w" ${TIF})  
originalHeight=$(magick identify -format "%h" ${TIF})

# Generate a random X and Y coordinate for the crop origin
# Ensure the crop area does not exceed the dimensions of the original image
maxX=$((originalWidth - image_width))
maxY=$((originalHeight - image_height))
x=$((RANDOM % maxX))
y=$((RANDOM % maxY))

# Perform the crop
magick convert ${TIF} -crop ${image_width}x${image_height}+${x}+${y} ${tmp_file}

# convert to jpeg
convert ${tmp_file} -quality ${JPEG_QUALITY} ${tmp_file}

######
## SCALE BAR
# Scale bar dimensions and position

### 200 um scale bar
scale_bar_length=615
scale_bar_height=20
bar_text="200 um"
text_pos_factor=2.2

## 100 um scale bar
scale_bar_length=308
scale_bar_height=20
bar_text="100 um"
text_pos_factor=2.8

# Calculate margins as 20% of the dimensions
right_margin=$(echo "$image_width * 0.05" | bc)
bottom_margin=$(echo "$image_height * 0.05" | bc)

# Calculate positions with bc and convert to integers
scale_bar_x=$(echo "$image_width - $scale_bar_length - $right_margin" | bc | cut -d'.' -f1)
scale_bar_y=$(echo "$image_height - $scale_bar_height - $bottom_margin" | bc | cut -d'.' -f1)

# Text position calculations
text_x=$(echo "$scale_bar_x + $scale_bar_length / $text_pos_factor" | bc | cut -d'.' -f1)
text_y=$(echo "$scale_bar_y - 20" | bc | cut -d'.' -f1) # 20 pixels above the scale bar

# Echo calculations
echo "Scale bar will be drawn from ($scale_bar_x, $scale_bar_y) to ($(($scale_bar_x + $scale_bar_length)), $(($scale_bar_y + $scale_bar_height)))"
echo "Text will be placed at ($text_x, $text_y)"

# Draw the scale bar and add the label
magick ${tmp_file} \
-fill white \
-draw "rectangle $scale_bar_x,$scale_bar_y $((scale_bar_x + scale_bar_length)),$((scale_bar_y + scale_bar_height))" \
-fill white -pointsize 30 \
-draw "text $text_x,$text_y '${bar_text}'" \
${tmp_file}

########
# LEGEND
########
# Rectangle dimensions (10% of the image height, full width)
rectangle_height=$(echo "$image_height * 0.1" | bc | cut -d'.' -f1)

# Channels and their respective colors
channels=("Channel1" "Channel2" "Channel3" "Channel4" "Channel5")  # Example channel names
colors=(
    "255,0,0"    # Red
    "0,255,0"    # Green
    "0,0,255"    # Blue
    "255,255,0"  # Yellow
    "255,165,0"  # Orange
)

# Calculate the number of text boxes from the channels array
num_text_boxes=${#channels[@]}

# Verify that the number of colors matches the number of channels
if [ ${#colors[@]} -ne $num_text_boxes ]; then
    echo "Error: The number of colors does not match the number of channels." >&2
    exit 1
fi

# Calculate the spacing for the text based on the number of text boxes
text_spacing=$(echo "$image_width / ($num_text_boxes + 1)" | bc)

# Create a new image with the rectangle
magick convert ${tmp_file} -fill black -draw "rectangle 0,0 $image_width,$rectangle_height" ${tmp_file}

# Loop through the channels and draw each one with its color
for ((i=0; i<num_text_boxes; i++)); do
    # Calculate the center position for each text box
    center_position=$(echo "($text_spacing / 2) + ($text_spacing * $i)" | bc)
    
    color=${colors[$i]}
    channel=${channels[$i]}
    
    # Draw text
    magick convert ${tmp_file} -fill "rgb($color)" -font ${FONT} -pointsize ${FONTSIZE} \
    -gravity NorthWest -annotate +$center_position+$(($rectangle_height / 2 - 15)) "$channel" ${tmp_file}
done

# Save the final image
mv ${tmp_file} tmp.jpg #${OUTJPG}
