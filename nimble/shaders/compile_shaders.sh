#!/bin/bash
# ============================================================================
# Shader Compilation Script
# ============================================================================
# This script compiles GLSL shaders (frag/vert) to SPIR-V for use with SDL3 GPU API.
# Requires: glslangValidator (from Vulkan SDK or glslang-tools)
#
# Usage: ./compile_shaders.sh[output_dir]
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-$SCRIPT_DIR/spirv}"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for glslangValidator
if ! command -v glslangValidator &> /dev/null; then
    echo "Error: glslangValidator not found."
    echo "Install Vulkan SDK or glslang-tools:"
    echo "  Ubuntu/Debian: sudo apt install glslang-tools"
    echo "  Arch: sudo pacman -S glslang"
    echo "  macOS: brew install glslang"
    exit 1
fi

echo "Compiling shaders to SPIR-V..."

# Compile each fragment and vertex shader
for shader in "$SCRIPT_DIR"/*.frag "$SCRIPT_DIR"/*.vert; do
    # Check if file exists (in case there are no matches for the glob)
    if [ -f "$shader" ]; then
        # Get the filename with its extension (e.g., "blur.frag" or "default.vert")
        filename=$(basename "$shader")
        output="$OUTPUT_DIR/${filename}.spv"
        
        echo "  Compiling: $shader -> $output"
        
        # Compile to SPIR-V Binary
        # -V: Compile to Vulkan SPIR-V
        # -o: Output file
        glslangValidator -V -o "$output" "$shader"
        
        if [ $? -eq 0 ]; then
            echo "    Success: $output"
        else
            echo "    Failed: $shader"
            exit 1
        fi
    fi
done

echo ""
echo "All shaders compiled successfully to: $OUTPUT_DIR"
echo ""
echo "Generated SPIR-V files:"
ls -la "$OUTPUT_DIR"/*.spv