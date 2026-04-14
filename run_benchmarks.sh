#!/bin/bash

# Define input and output folders
INPUT_FOLDER="${1:-/home/jason/research/TRO_sub/baseline/cora/new_data}"
BASE_OUTPUT_FOLDER="4_13"
ITER_INFO_FOLDER="${BASE_OUTPUT_FOLDER}/iteration_info"
TRAJECTORY_FOLDER="${BASE_OUTPUT_FOLDER}/trajectory"

# Create output folders if they don't exist
mkdir -p "$ITER_INFO_FOLDER"
mkdir -p "$TRAJECTORY_FOLDER"

# Path to the cora_example executable
EXECUTABLE="./cmake-build-release/bin/cora_example"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable $EXECUTABLE not found. Please build the project first."
    exit 1
fi

# Find all .pyfg files and run the executable
find "$INPUT_FOLDER" -name "*.pyfg" | while read -r pyfg_file; do
    # Create a unique filename based on the relative path to avoid collisions
    # Use realpath to handle absolute paths correctly for rel_path
    abs_input_folder=$(realpath "$INPUT_FOLDER")
    abs_pyfg_file=$(realpath "$pyfg_file")
    
    if [[ -d "$INPUT_FOLDER" ]]; then
        rel_path="${abs_pyfg_file#$abs_input_folder/}"
    else
        rel_path=$(basename "$abs_pyfg_file")
    fi
    
    result_base="${rel_path//\//_}"
    result_base="${result_base%.pyfg}"
    
    iter_info_file="${ITER_INFO_FOLDER}/${result_base}.csv"
    trajectory_file="${TRAJECTORY_FOLDER}/${result_base}.g2o"

    echo "Processing $pyfg_file"
    echo "  Iteration info: $iter_info_file"
    echo "  Trajectory:     $trajectory_file"
    
    # Run the cora_example executable
    "$EXECUTABLE" "$pyfg_file" "$iter_info_file" "$trajectory_file"
    
    if [ $? -eq 0 ]; then
        echo "Successfully processed $pyfg_file"
    else
        echo "Failed to process $pyfg_file"
    fi
done

echo "Benchmarking complete. Results saved in $BASE_OUTPUT_FOLDER"
