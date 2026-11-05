# Charcoal application chrome

Use #212121 launch windows, #2F2F2F raised surfaces, #383838 interactive
surfaces, #454545 borders, #F4F4F4 text and #B4B4B4 secondary/focus colors.
These match the shared android-NDK charcoal contract. Existing resource
packagers consume the app-owned XML without adding another toolchain.

The homepage, Android launch window, placement controls and polynomial overlays
adopt these roles. Portrait shaders, complex-function color mapping, scientific
images, marker positions, factor state, gestures, package, version and signer
are preserved. The scientific portrait is an explicit exception to any whole
screen darkness heuristic; test application chrome separately.

Exact-source build, signed packaging, live Android screenshots, both system
night modes and physical MIRO C67 acceptance remain pending. This branch must
not publish a release, deploy Pages or replace an installed application.
