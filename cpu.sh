#!/bin/bash
for i in {1..100}
do
    inputs="$inputs$((1 + $RANDOM % 3))";
done
./a.out << EOF
"$inputs"
EOF