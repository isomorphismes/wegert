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

At source23beee5, native build/emulation and direct DEX/JNI workflows pass.
The F-Droid direct reproducible build and signed device candidate pass; its
separate buildserver lane cannot pull the recorded registry container image.
Both system night modes, API29 and physical MIRO C67 acceptance remain pending.
This branch must not publish a release, deploy Pages or replace an installed app.

The ICK source lane's Grease host artifact12192502970 expired. It now retrieves
artifact13062637698 with its independently verified archive digest. The actual
Grease runtime SHA256 remains
7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c;
the launcher digest also remains unchanged. This repairs delivery of the same
runtime and does not substitute another shell or language implementation.
