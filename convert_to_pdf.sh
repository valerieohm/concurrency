#!/bin/bash

# Create clean copies first (don't overwrite originals)
for file in doc/*.md; do
    if [ -f "$file" ]; then
        basename_file=$(basename "$file")
        echo "Processing $file -> doc/${basename_file%.md}_clean.md"
        sed 's/[^\x00-\x7F]//g' "$file" > "doc/${basename_file%.md}_clean.md"
    fi
done

# Convert clean copies to PDF
for file in doc/*_clean.md; do
    if [ -f "$file" ]; then
        basename_file=$(basename "$file")
        echo "Converting $file -> doc/${basename_file%_clean.md}.pdf"
        pandoc "$file" -o "doc/${basename_file%_clean.md}.pdf"
    fi
done

# Remove temporary clean files
echo "Cleaning up temporary files..."
#rm -f doc/*_clean.md

echo "PDF conversion complete!"
