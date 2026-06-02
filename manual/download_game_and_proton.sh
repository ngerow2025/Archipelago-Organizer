#!/bin/bash

# This script downloads Proton 10 and a specific version of A Dance of Fire and Ice (ADOFAI)
# using DepotDownloader.

# Define target directories
PROTON_DIR="proton-10.0"
ADOFAI_DIR="adofai-older"

# Clean and create target directories
echo "Preparing directories..."
rm -rf "$PROTON_DIR" "$ADOFAI_DIR"
mkdir -p "$PROTON_DIR" "$ADOFAI_DIR"

# Make sure DepotDownloader is executable
chmod +x ./DepotDownloader/DepotDownloader

# Download Proton 10
echo "Downloading Proton 10..."
./DepotDownloader/DepotDownloader -app 3658110 -username nat_than -remember-password

# Download ADOFAI
echo "Downloading ADOFAI..."
./DepotDownloader/DepotDownloader -app 977950 -depot 977953 -branch older -username nat_than -remember-password

# Move downloaded files
echo "Moving downloaded files..."
mv depots/3658111/*/* "$PROTON_DIR/"
mv depots/977953/*/* "$ADOFAI_DIR/"


echo "Downloads and setup complete."
echo "Proton is in '$PROTON_DIR'"
echo "ADOFAI is in '$ADOFAI_DIR'"

