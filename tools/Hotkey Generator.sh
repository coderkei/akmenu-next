#!/bin/bash

echo "-------------------------"
printf "| %-7s | %-8s |\n" "Key" "Value"
echo "-------------------------"
printf "| %-7s | %-8d |\n" "A" 0
printf "| %-7s | %-8d |\n" "B" 1
printf "| %-7s | %-8d |\n" "Select" 2
printf "| %-7s | %-8d |\n" "Start" 3
printf "| %-7s | %-8d |\n" "Right" 4
printf "| %-7s | %-8d |\n" "Left" 5
printf "| %-7s | %-8d |\n" "Up" 6
printf "| %-7s | %-8d |\n" "Down" 7
printf "| %-7s | %-8d |\n" "R" 8
printf "| %-7s | %-8d |\n" "L" 9
printf "| %-7s | %-8d |\n" "X" 10
printf "| %-7s | %-8d |\n" "Y" 11
printf "| %-7s | %-8d |\n" "Hinge" 12
printf "| %-7s | %-8d |\n" "Debug" 13
echo "-------------------------"

echo "Please enter between 3 and 14 numbers (0-13), separated by spaces:"
read -a values

len=${#values[@]}

if (( len < 3 )); then
    echo "ERROR: Please enter at least 3 values"
    exit 1
fi

if (( len > 14 )); then
    echo "ERROR: Please enter no more than 14 values"
    exit 1
fi

result=0

for val in "${values[@]}"; do
    if (( val < 0 || val > 13 )); then
        echo "ERROR: Value $val is out of range (0-13)"
        exit 1
    fi
    # Calculate 2^val and OR with result
    (( result |= (1 << val) ))
done

printf "Your converted hotkey value is %X. Please place this in ndsbs.ini in the hotkey value\n" $result
