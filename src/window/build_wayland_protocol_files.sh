#!/bin/bash

echo "Generating wayland protocol files..."
mkdir -p wayland_protocols
for filename in wayland_protocol_descriptions/*.xml; do
    wayland-scanner client-header < "$filename" > "wayland_protocols/$(basename "$filename" .xml).tmp"
    # Remove #include "wayland-client.h"
    sed '/include "wayland-client.h"/d' "wayland_protocols/$(basename "$filename" .xml).tmp" > "wayland_protocols/$(basename "$filename" .xml).h"
    rm "wayland_protocols/$(basename "$filename" .xml).tmp"

    wayland-scanner private-code < "$filename" > "wayland_protocols/$(basename "$filename" .xml).c"
done

#cp "wayland_protocol_descriptions/wayland-util.h" "wayland_protocols/"

echo "Finished generation of wayland protocol files"
