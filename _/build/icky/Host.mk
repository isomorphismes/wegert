ICK ?=
ICK_FLAGS ?= -fno-link-libatomic
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUT ?= $(ROOT)/build/ick-host
COMMON := -std=c11 -Wall -Wextra -Wpedantic -Werror

ifeq ($(strip $(ICK)),)
$(error ICK must name the qualified native compiler)
endif

.PHONY: test renderer
test:
	mkdir -p "$(OUT)"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/tests/factor_drag_test.c -lm -o "$(OUT)/factor-drag"
	"$(OUT)/factor-drag"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) -O2 $(ROOT)/complex_math_fallback.c $(ROOT)/tests/complex_math_test.c -lm -o "$(OUT)/complex-math"
	"$(OUT)/complex-math"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/wegert_function.c $(ROOT)/wegert_view.c $(ROOT)/wegert_scene.c $(ROOT)/tests/test_wegert_scene.c -lm -o "$(OUT)/scene"
	"$(OUT)/scene"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/wegert_function.c $(ROOT)/tests/test_wegert_function.c -o "$(OUT)/function"
	"$(OUT)/function"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/wegert_placement_controls.c $(ROOT)/tests/test_wegert_placement_controls.c -lm -o "$(OUT)/placement-controls"
	"$(OUT)/placement-controls"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/tests/test_gesture_state.c -lm -o "$(OUT)/gesture"
	"$(OUT)/gesture"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/tests/test_factor_state.c -o "$(OUT)/factor"
	"$(OUT)/factor"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/tests/test_factor_snap.c -lm -o "$(OUT)/factor-snap"
	"$(OUT)/factor-snap"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(ROOT)/tests/test_polynomial_text.c $(ROOT)/complex_math_fallback.c -lm -o "$(OUT)/polynomial-text"
	"$(OUT)/polynomial-text"

renderer:
	mkdir -p "$(OUT)"
	"$(ICK)" $(ICK_FLAGS) $(COMMON) $(CPPFLAGS) $(ROOT)/wegert_function.c $(ROOT)/wegert_view.c $(ROOT)/wegert_scene.c $(ROOT)/wegert_gles.c $(ROOT)/wegert_portrait_renderer_gles.c $(ROOT)/wegert_offscreen_gles.c $(ROOT)/wegert_render_frames.c $(LDFLAGS) -lEGL -lGLESv2 -lm -o "$(OUT)/wegert-render-frames"
