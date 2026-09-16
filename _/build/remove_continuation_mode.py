from pathlib import Path
import re


def read(path):
    return Path(path).read_text()


def write(path, text):
    Path(path).write_text(text)


def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one exact match, found {count}: {old[:100]!r}")
    write(path, text.replace(old, new, 1))


def sub_once(path, pattern, replacement, flags=0):
    text = read(path)
    updated, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f"{path}: expected one regex match, found {count}: {pattern[:120]!r}")
    write(path, updated)


# Gesture state: retain ordinary controls, factor gestures, pinch and reset.
replace_once(
    "code/gesture_state.h",
    "    GESTURE_CLEAR_BUTTON,\n    GESTURE_VIEW_BUTTON,\n    GESTURE_BLOCKED\n",
    "    GESTURE_CLEAR_BUTTON,\n    GESTURE_BLOCKED\n",
)
replace_once(
    "code/gesture_state.h",
    "    return gesture == GESTURE_CLEAR_BUTTON ||\n        gesture == GESTURE_VIEW_BUTTON ||\n        gesture == GESTURE_BLOCKED;\n",
    "    return gesture == GESTURE_CLEAR_BUTTON ||\n        gesture == GESTURE_BLOCKED;\n",
)
sub_once(
    "code/gesture_state.h",
    r"\nstatic bool gesture_view_release_toggles\(enum gesture_kind gesture, bool released_inside\) \{.*?\n\}\n",
    "",
    re.S,
)
sub_once(
    "code/gesture_state.h",
    r"\nstatic bool gesture_touch_can_capture_factor\(bool continuation_view\) \{.*?\n\}\n",
    "",
    re.S,
)

# Shader: ordinary phase portrait plus factor markers only.
shader = read("code/wegert.frag.in")
shader = shader.replace("#define MAX_CONTINUATION_STEPS 24\n", "")
shader = shader.replace(
    "uniform int u_view_kind;\nuniform int u_continuation_count;\n"
    "uniform vec2 u_continuation_centers[MAX_CONTINUATION_STEPS];\n"
    "uniform float u_continuation_radii[MAX_CONTINUATION_STEPS];\n",
    "",
)
shader, count = re.subn(
    r"\nfloat distance_to_segment\(vec2 point, vec2 start, vec2 finish\) \{.*?\n\}\n",
    "",
    shader,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.frag.in: distance_to_segment removal failed")
shader, count = re.subn(
    r"\n    if \(u_view_kind == 1\) \{.*?\n    \}\n\n(?=    for \(int index = 0; index < MAX_FACTORS; \+\+index\) \{\n        if \(index >= u_zero_count\))",
    "\n",
    shader,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.frag.in: continuation render block removal failed")
if "continuation" in shader.lower() or "u_view_kind" in shader:
    raise SystemExit("code/wegert.frag.in: continuation residue remains")
write("code/wegert.frag.in", shader)

# Overlay UI: retain formula and clear, remove view switch entirely.
overlay = read("code/polynomial_overlay.h")
overlay, count = re.subn(
    r"\nstatic void view_button_origin\(const struct engine \*engine, float \*x, float \*y\) \{.*?\n\}\n",
    "",
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: view_button_origin removal failed")
old = "    glGenTextures(1, &engine->view_button_texture);\n    overlay_configure_texture(engine->view_button_texture);\n"
if overlay.count(old) != 1:
    raise SystemExit("code/polynomial_overlay.h: view texture initialization not unique")
overlay = overlay.replace(old, "", 1)
overlay, count = re.subn(
    r"\nstatic bool view_button_rebuild_texture\(struct engine \*engine\) \{.*?\n\}\n\n(?=static bool polynomial_overlay_rebuild_texture)",
    "\n",
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: view builder removal failed")

overlay, count = re.subn(
    r"static void polynomial_overlay_draw\(struct engine \*engine\) \{.*?\n\}\n\n(?=static void polynomial_overlay_destroy)",
    '''static void polynomial_overlay_draw(struct engine *engine) {
    if (!polynomial_overlay_initialize(engine)) {
        return;
    }
    if (engine->overlay_dirty && !polynomial_overlay_rebuild_texture(engine)) {
        return;
    }
    if (
        (engine->clear_button_width <= 0 || engine->clear_button_height <= 0) &&
        !clear_button_rebuild_texture(engine)
    ) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(engine->overlay_program);
    glUniform2f(engine->overlay_resolution_location, (float)engine->width, (float)engine->height);

    float clear_button_x = 0.0f;
    float clear_button_y = 0.0f;
    clear_button_origin(engine, &clear_button_x, &clear_button_y);

    overlay_draw_texture(
        engine,
        engine->overlay_texture,
        engine->overlay_width,
        engine->overlay_height,
        16.0f,
        16.0f
    );
    overlay_draw_texture(
        engine,
        engine->clear_button_texture,
        engine->clear_button_width,
        engine->clear_button_height,
        clear_button_x,
        clear_button_y
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

''',
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: draw replacement failed")

overlay, count = re.subn(
    r"static void polynomial_overlay_destroy\(struct engine \*engine\) \{.*?\n\}\n\n(?=static bool polynomial_overlay_contains)",
    '''static void polynomial_overlay_destroy(struct engine *engine) {
    if (engine->overlay_texture != 0) {
        glDeleteTextures(1, &engine->overlay_texture);
        engine->overlay_texture = 0;
    }
    if (engine->clear_button_texture != 0) {
        glDeleteTextures(1, &engine->clear_button_texture);
        engine->clear_button_texture = 0;
    }
    if (engine->overlay_program != 0) {
        glDeleteProgram(engine->overlay_program);
        engine->overlay_program = 0;
    }
    engine->overlay_width = 0;
    engine->overlay_height = 0;
    engine->clear_button_width = 0;
    engine->clear_button_height = 0;
    engine->overlay_unavailable = false;
    engine->overlay_dirty = true;
}

''',
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: destroy replacement failed")
overlay, count = re.subn(
    r"static bool polynomial_overlay_contains\(const struct engine \*engine, float x, float y\) \{.*?\n\}\n",
    '''static bool polynomial_overlay_contains(const struct engine *engine, float x, float y) {
    return engine->overlay_width > 0 && engine->overlay_height > 0 &&
        x >= 16.0f && x < 16.0f + (float)engine->overlay_width &&
        y >= 16.0f && y < 16.0f + (float)engine->overlay_height;
}
''',
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: overlay hit-test replacement failed")
overlay, count = re.subn(
    r"\nstatic bool view_button_contains\(const struct engine \*engine, float x, float y\) \{.*?\n\}\n",
    "",
    overlay,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/polynomial_overlay.h: view hit-test removal failed")
if "continuation" in overlay.lower() or "view_button" in overlay or "VIEW_CONTINUATION" in overlay:
    raise SystemExit("code/polynomial_overlay.h: view-switch residue remains")
write("code/polynomial_overlay.h", overlay)

# Native app state/input.
app = read("code/wegert.c")
app = app.replace('#include "continuation_path.h"\n', '')
app, count = re.subn(
    r"\nenum view_kind \{\n    VIEW_WHOLE_PORTRAIT,\n    VIEW_CONTINUATION\n\};\n",
    "",
    app,
    count=1,
)
if count != 1:
    raise SystemExit("code/wegert.c: view enum removal failed")
for line in (
    "    GLint view_kind_location;\n",
    "    GLint continuation_count_location;\n",
    "    GLint continuation_centers_location;\n",
    "    GLint continuation_radii_location;\n",
    "    GLuint view_button_texture;\n",
    "    int view_button_width;\n",
    "    int view_button_height;\n",
    "    bool view_button_dirty;\n",
    "    enum view_kind view_kind;\n",
    "    struct continuation_path continuation;\n",
):
    if app.count(line) != 1:
        raise SystemExit(f"code/wegert.c: expected one field line: {line!r}")
    app = app.replace(line, "", 1)
app, count = re.subn(
    r"\nstatic void clear_continuation_path\(struct engine \*engine\) \{.*?(?=\nstatic void initialize_function)",
    "\n",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: continuation view functions removal failed")
app = app.replace("    continuation_path_clear(&engine->continuation);\n", "")
app = app.replace("    engine->view_kind = VIEW_WHOLE_PORTRAIT;\n", "")
app = app.replace("    engine->view_button_dirty = true;\n", "")
app = app.replace(
    '    LOGI("default function, camera, view, and continuation path reset");\n',
    '    LOGI("default function and camera reset");\n',
)
app, count = re.subn(
    r"\n    engine->view_kind_location = glGetUniformLocation\(engine->program, \"u_view_kind\"\);.*?(?=\n    GLuint placement_vertex_shader)",
    "\n",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: continuation uniform lookup removal failed")
app, count = re.subn(
    r"\n    glUniform1i\(engine->view_kind_location,.*?(?=\n\n    glBindVertexArray\(engine->vao\);)",
    "",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: continuation uniform upload removal failed")
app, count = re.subn(
    r"\nstatic void add_continuation_center\(struct engine \*engine, float x, float y\) \{.*?(?=\nstatic float placement_control_radius)",
    "\n",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: continuation input function removal failed")
replace = "                set_whole_portrait_view(engine);\n"
if app.count(replace) != 1:
    raise SystemExit("code/wegert.c: placement view-reset line not unique")
app = app.replace(replace, "", 1)
app, count = re.subn(
    r"\n            if \(view_button_contains\(engine, x, y\)\) \{.*?\n            \}\n",
    "\n",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: view button input removal failed")
app, count = re.subn(
    r"            struct factor_target target = \{\n.*?            if \(gesture_touch_can_capture_factor\(engine->view_kind == VIEW_CONTINUATION\)\) \{\n                target = factor_target_at\(engine, x, y\);\n            \}\n",
    "            struct factor_target target = factor_target_at(engine, x, y);\n",
    app,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("code/wegert.c: factor capture restoration failed")
replace_once_text = '''                if (engine->view_kind == VIEW_CONTINUATION) {
                    add_continuation_center(engine, x, y);
                } else if (engine->placement_kind == FACTOR_POLE) {
                    add_pole(engine, x, y);
                } else {
                    add_zero(engine, x, y);
                }
'''
replacement_text = '''                if (engine->placement_kind == FACTOR_POLE) {
                    add_pole(engine, x, y);
                } else {
                    add_zero(engine, x, y);
                }
'''
if app.count(replace_once_text) != 1:
    raise SystemExit("code/wegert.c: tap action continuation branch not unique")
app = app.replace(replace_once_text, replacement_text, 1)
clear_old = '''                if (engine->view_kind == VIEW_CONTINUATION) {
                    clear_continuation_path(engine);
                    LOGI("continuation path cleared");
                } else {
                    clear_function(engine);
                    LOGI("whole portrait factors cleared");
                }
            } else if (gesture_view_release_toggles(
                engine->gesture,
                view_button_contains(
                    engine,
                    AMotionEvent_getX(event, 0),
                    AMotionEvent_getY(event, 0)
                )
            )) {
                toggle_view(engine);
'''
clear_new = '''                clear_function(engine);
                LOGI("factors cleared");
'''
if app.count(clear_old) != 1:
    raise SystemExit("code/wegert.c: clear/view-switch release block not unique")
app = app.replace(clear_old, clear_new, 1)
for residue in ("continuation", "view_button", "VIEW_CONTINUATION", "view_kind"):
    if residue.lower() in app.lower():
        raise SystemExit(f"code/wegert.c: residue remains: {residue}")
write("code/wegert.c", app)

# Host tests retain ordinary gestures and snapping/cancellation.
replace_once(
    "_/build/tests/test_gesture_state.c",
    "    enum gesture_kind ui_holds[] = {\n        GESTURE_BLOCKED,\n        GESTURE_CLEAR_BUTTON,\n        GESTURE_VIEW_BUTTON\n    };\n    for (int index = 0; index < 3; ++index) {\n",
    "    enum gesture_kind ui_holds[] = {\n        GESTURE_BLOCKED,\n        GESTURE_CLEAR_BUTTON\n    };\n    for (int index = 0; index < 2; ++index) {\n",
)
sub_once(
    "_/build/tests/test_gesture_state.c",
    r"\n    assert\(!gesture_view_release_toggles.*?assert\(!gesture_touch_can_capture_factor\(true\)\);\n",
    "",
    re.S,
)
replace_once("_/build/tests/test_factor_snap.c", '#include "../continuation_path.h"\n', "")
sub_once(
    "_/build/tests/test_factor_snap.c",
    r"\n    float zeros\[MAX_FACTORS\]\[2\] = \{\{0\.0f, 0\.0f\}\};.*?assert\(path\.radii\[0\] > 0\.0f\);\n",
    "",
    re.S,
)

# CI should exercise ordinary Wegert only.
workflow = read(".github/workflows/android.yml")
workflow = workflow.replace(
    "          cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_continuation_path.c -lm -o /tmp/wegert-continuation-test\n"
    "          /tmp/wegert-continuation-test\n",
    "",
)
marker = "          script: |\n"
start = workflow.index(marker) + len(marker)
end_marker = "\n\n      - uses: actions/upload-artifact@v4"
end = workflow.index(end_marker, start)
smoke = '''            adb shell wm size ${{ matrix.size }}
            adb shell wm density ${{ matrix.density }}
            adb install -r artifacts/app-debug.apk
            adb logcat -c
            adb shell am start -W -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n org.isomorphisms.wegert/android.app.NativeActivity
            sleep 4
            adb logcat -d > wegert-${{ matrix.device }}.log
            adb exec-out screencap -p > wegert-${{ matrix.device }}.png
            adb shell pidof -s org.isomorphisms.wegert | tr -d '\\r' | grep -Eq '^[0-9]+$'
            awk '/EGL surface ready:/ { split($NF, dimensions, "x"); width=dimensions[1]; height=dimensions[2] } END { exit !(width > 0 && height > 0) }' wegert-${{ matrix.device }}.log

            # Drag the existing zero at z=1.
            adb shell input swipe "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[1] / 2 + dimensions[2] / 7) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[2] / 2) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[1] / 2 + dimensions[2] / 7 + 80) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[2] / 2) }' wegert-${{ matrix.device }}.log)" 350
            sleep 1

            # Place a pole and keep the rational-function formula exercised.
            adb shell input tap "$(awk '/placement control centers:/ { value=$NF } END { sub(/^pole=/, "", value); split(value, coordinates, ","); print coordinates[1] }' wegert-${{ matrix.device }}.log)" "$(awk '/placement control centers:/ { value=$NF } END { sub(/^pole=/, "", value); split(value, coordinates, ","); print coordinates[2] }' wegert-${{ matrix.device }}.log)"
            adb shell input tap "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(3 * dimensions[1] / 4) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(2 * dimensions[2] / 5) }' wegert-${{ matrix.device }}.log)"
            sleep 1

            # Pan from empty portrait space.
            adb shell input swipe "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[1] / 4) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[2] / 2) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[1] / 4 + 80) }' wegert-${{ matrix.device }}.log)" "$(awk '/EGL surface ready:/ { split($NF, dimensions, "x") } END { print int(dimensions[2] / 2) }' wegert-${{ matrix.device }}.log)" 350
            sleep 1

            adb logcat -d > wegert-${{ matrix.device }}.log
            adb exec-out screencap -p > wegert-${{ matrix.device }}.png
            adb shell pidof -s org.isomorphisms.wegert | tr -d '\\r' | grep -Eq '^[0-9]+$'
            grep -Fq 'factor drag completed: kind=zero index=0' wegert-${{ matrix.device }}.log
            grep -Fq 'function overlay: (' wegert-${{ matrix.device }}.log
            grep -Fq ' ÷' wegert-${{ matrix.device }}.log
            ! grep -Eiq 'shader compilation failed|program link failed|eglInitialize failed|could not choose GLES3 EGL config|could not create EGL surface/context|eglMakeCurrent failed' wegert-${{ matrix.device }}.log

            adb shell input tap "$(awk '/clear control center:/ { value=$(NF-1) } END { print value }' wegert-${{ matrix.device }}.log)" "$(awk '/clear control center:/ { value=$NF } END { print value }' wegert-${{ matrix.device }}.log)"
            sleep 1
            adb logcat -d > wegert-${{ matrix.device }}.log
            grep -Fq 'factors cleared' wegert-${{ matrix.device }}.log
            adb shell input keyevent KEYCODE_BACK
            sleep 1
            ! adb shell dumpsys activity activities | grep -E 'mResumedActivity|topResumedActivity' | grep -Fq 'org.isomorphisms.wegert'
'''
workflow = workflow[:start] + smoke + workflow[end:]
if "continuation" in workflow.lower():
    raise SystemExit(".github/workflows/android.yml: continuation residue remains")
write(".github/workflows/android.yml", workflow)

# README: preserve the ownership boundary but remove the local continuation product mode.
readme = read("README.md")
controls = '''## First playable controls

- tap the ○ or × control, then tap the portrait to add that kind of factor (up to 64 each)
- one-finger drag from an existing marker: move that factor
- one-finger drag from empty portrait space: move the visible complex domain
- pinch: zoom the visible domain
- `clear`: remove every zero and pole without moving the visible domain
- three-finger tap: restore `g(z) = (z - 1)(z - 2)(z - 5)` and recenter the camera
- Android Back: leave the activity using the system control rather than an in-app exit button

The initial view is centered at the ordinary complex zero. Zeros are shown as dark rings with light centers; poles are shown as dark crosses.

Touching near an opposite marker snaps to that marker's stored coordinate using a density-aware touch target: placing a zero there removes one pole instead of storing the zero, and placing a pole on a zero works the same way. Cancellation is one-for-one, including repeated factors. This screen-space touch aid does not change the exact-coordinate rule for programmatic factor values, so merely nearby stored factors remain distinct.

Clear activates only when the first finger is released inside the control. Additional fingers cancel that pending control action and cannot turn it into a pinch or three-finger reset. Pinch and three-finger reset remain available when the gesture begins on the portrait.

'''
readme, count = re.subn(
    r"## First playable controls\n.*?(?=## Colouring\n)",
    controls,
    readme,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("README.md: controls/continuation section replacement failed")
readme = readme.replace(
    "The host-side continuation, gesture, pinch-zoom, factor-drag, canonical-factor, touch-snap, complex-arithmetic, and formula-formatting rules can be checked from the same build directory without an Android toolchain:",
    "The host-side gesture, pinch-zoom, factor-drag, canonical-factor, touch-snap, complex-arithmetic, and formula-formatting rules can be checked from the same build directory without an Android toolchain:",
)
readme = readme.replace(
    "cc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_continuation_path.c -lm -o /tmp/wegert-continuation-test\n/tmp/wegert-continuation-test\n",
    "",
)
old_emulation = "Each emulator installs and launches Wegert, drags an existing zero in the whole portrait, places a finite pole away from the camera, and enters continuation view. The smoke test requires positive finite radii for both the camera seed and an accepted center inside that disc, then requires rejection of a tap outside the new disc. It pans after those geometry checks, returns to the whole portrait, activates clear, leaves through Android Back, fails on EGL/shader/link/fatal errors, and saves a screenshot plus application log."
new_emulation = "Each emulator installs and launches Wegert, drags an existing zero, places a finite pole, pans from empty portrait space, checks the rational-function overlay, activates clear, leaves through Android Back, fails on EGL/shader/link/fatal errors, and saves a screenshot plus application log."
if old_emulation not in readme:
    raise SystemExit("README.md: emulator acceptance paragraph not found")
readme = readme.replace(old_emulation, new_emulation, 1)
for residue in ("## Continuation view", "`continuation`", "Taylor-disc"):
    if residue in readme:
        raise SystemExit(f"README.md: active continuation residue remains: {residue}")
write("README.md", readme)

# Homepage and current store copy.
index = read("index.html")
index = index.replace(
    "          drag them through the complex plane, pan, zoom, and inspect continuation geometry.\n",
    "          drag them through the complex plane, pan, zoom, and watch the portrait respond immediately.\n",
)
index = index.replace("      <span>Continuation view</span>\n", "")
old_card = '''        <article class="card">
          <div class="card_symbol" aria-hidden="true">Σ</div>
          <h3>Reveal Taylor discs</h3>
          <p>Continuation view shows bounded Taylor-disc geometry for the already-defined rational function.</p>
        </article>'''
new_card = '''        <article class="card">
          <div class="card_symbol" aria-hidden="true">f</div>
          <h3>See the rational formula</h3>
          <p>The formula overlay follows the same zeros and poles as the portrait, including denominator factors.</p>
        </article>'''
if old_card not in index:
    raise SystemExit("index.html: continuation feature card not found")
index = index.replace(old_card, new_card, 1)
if "continuation" in index.lower() or "taylor disc" in index.lower():
    raise SystemExit("index.html: continuation product copy remains")
write("index.html", index)

write(
    "_/build/fastlane/metadata/android/en-US/full_description.txt",
    "zero & infinity is an interactive phase-portrait explorer for complex rational functions.\n\n"
    "Choose the ○ or × control, then tap the portrait to add a zero or pole. Drag an existing marker to move that factor, drag empty portrait space to pan the complex plane, and pinch to zoom. Clear removes every factor without moving the camera, and a three-finger tap restores the starting function and camera.\n\n"
    "Touching near an opposite marker snaps to that stored coordinate so equal zero/pole factors cancel one-for-one. The displayed rational function uses ÷ followed by each denominator factor.\n\n"
    "The phase portrait is rendered directly on the GPU using domain coloring: hue shows phase and repeating lightness bands show modulus. Android's system Back control leaves the app; no separate exit button occupies the portrait.\n\n"
    "The Android app is native C with EGL and OpenGL ES 3. It runs entirely on the device and requests no network permission.\n",
)
write(
    "_/build/fastlane/metadata/android/en-US/changelogs/101.txt",
    "Adds the rational-function formula and a visible clear control. Improves pole placement, factor dragging, exact zero/pole cancellation, and multi-touch handling; raises the factor limit, adds 32-bit ARM support, and makes release checks reproducible from the exact tested source.\n",
)
replace_once(
    "_/build/fdroid/README.md",
    "factor placement/dragging, pan, pinch, continuation, clear, and Android Back",
    "factor placement/dragging, pan, pinch, clear, and Android Back",
)

# Delete active continuation implementation/aliases/test; history remains in Git.
for path in (
    "code/continuation_path.h",
    "continuation_path.h",
    "_/build/continuation_path.h",
    "_/build/tests/test_continuation_path.c",
):
    p = Path(path)
    if not (p.exists() or p.is_symlink()):
        raise SystemExit(f"expected continuation file/alias is absent: {path}")
    p.unlink()

# Strong residue checks on active product paths.
for path in (
    "code/wegert.c",
    "code/wegert.frag.in",
    "code/polynomial_overlay.h",
    "code/gesture_state.h",
    "_/build/tests/test_gesture_state.c",
    "_/build/tests/test_factor_snap.c",
    ".github/workflows/android.yml",
    "index.html",
    "_/build/fastlane/metadata/android/en-US/full_description.txt",
    "_/build/fastlane/metadata/android/en-US/changelogs/101.txt",
    "_/build/fdroid/README.md",
):
    if "continuation" in read(path).lower():
        raise SystemExit(f"continuation residue remains in active product path: {path}")

# README may mention Analytic Continuation/Lacunary as external ownership, but not a local mode.
readme = read("README.md")
for residue in ("## Continuation view", "`continuation`", "Taylor-disc", "view switch"):
    if residue in readme:
        raise SystemExit(f"README active-mode residue: {residue}")

print("continuation mode removed from active Wegert product paths")
