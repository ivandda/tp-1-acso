#!/bin/bash
# Create output directory if it doesn't exist
OUTPUT_DIR=tests_outputs
mkdir -p "$OUTPUT_DIR"

# Redirect stdout and stderr to final_results.txt inside the output directory
exec > "$OUTPUT_DIR"/final_results.txt 2>&1

# Loop through each test file in the inputs directory
for test in ../inputs/tests_1/*.x; do
    echo "Running test for $test"
    
    # Get the base name of the test file without extension for unique output filenames
    TEST_NAME=$(basename "$test" .x)
    
    # Run the reference simulator and capture its output
    ./ref_sim_x86 "$test" <<EOF > "$OUTPUT_DIR"/ref_output_"$TEST_NAME".txt
go
rdump
quit
EOF

    # Run your simulator and capture its output
    ./sim "$test" <<EOF > "$OUTPUT_DIR"/sim_output_"$TEST_NAME".txt
go
rdump
quit
EOF

    # Filter outputs: extract only the section from the register dump onward
    sed -n '/^Current register\/bus values/,$p' "$OUTPUT_DIR"/ref_output_"$TEST_NAME".txt > "$OUTPUT_DIR"/ref_filtered_"$TEST_NAME".txt
    sed -n '/^Current register\/bus values/,$p' "$OUTPUT_DIR"/sim_output_"$TEST_NAME".txt > "$OUTPUT_DIR"/sim_filtered_"$TEST_NAME".txt

    # Compare the filtered outputs
    if diff -q "$OUTPUT_DIR"/ref_filtered_"$TEST_NAME".txt "$OUTPUT_DIR"/sim_filtered_"$TEST_NAME".txt > /dev/null; then
        echo "Test $test passed."
    else
        echo "Test $test failed. Differences:"
        diff "$OUTPUT_DIR"/ref_filtered_"$TEST_NAME".txt "$OUTPUT_DIR"/sim_filtered_"$TEST_NAME".txt
    fi
done
