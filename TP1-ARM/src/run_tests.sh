#!/bin/bash
# Redirect stdout and stderr to final_results.txt
exec > final_results.txt 2>&1

# Loop through each test file in the inputs directory
for test in ../inputs/tests_1/*.x; do
    echo "Running test for $test"
    
    # Run the reference simulator and capture its output
    ./ref_sim_x86 "$test" <<EOF > ref_output.txt
go
rdump
quit
EOF

    # Run your simulator and capture its output
    ./sim "$test" <<EOF > sim_output.txt
go
rdump
quit
EOF

    # Filter outputs: extract only the section from the register dump onward
    sed -n '/^Current register\/bus values/,$p' ref_output.txt > ref_filtered.txt
    sed -n '/^Current register\/bus values/,$p' sim_output.txt > sim_filtered.txt

    # Compare the filtered outputs
    if diff -q ref_filtered.txt sim_filtered.txt > /dev/null; then
        echo "Test $test passed."
    else
        echo "Test $test failed. Differences:"
        diff ref_filtered.txt sim_filtered.txt
    fi
done
