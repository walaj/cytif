#!/bin/bash

# Function to check if "magick" command is available
magick_validate() {
    if ! command -v magick &> /dev/null; then
        echo "Error: ImageMagick ('magick' command) is not installed or not in the PATH." >&2
        return 1
    fi
    return 0
}

# Function to validate the palette file format and content
palette_validate() {
    local palette_file="$1"
    local expected_header="number,name,r,g,b,lower,upper"
    local expected_num_columns=7
    
    if [ ! -f "$palette_file" ]; then
        echo "Error: Palette file does not exist." >&2
        return 1
    fi
    
    # Read and check the header
    read -r header < "$palette_file"
    if [ "$header" != "$expected_header" ]; then
        echo "Error: Palette file header does not match expected format." >&2
        return 1
    fi

    # Check each line for the correct number of elements
    local line_num=1
    while IFS=, read -r -a columns; do
        if [ ${#columns[@]} -ne $expected_num_columns ]; then
            echo "Error: Line $line_num in palette file does not have the correct number of elements." >&2
            return 1
        fi
        ((line_num++))
    done < <(tail -n +2 "$palette_file")  # Skip the header line

    return 0
}
