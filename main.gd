extends Control

const MAX_POINTS := 16
const Y_HALF_EXTENT := 2.5
const PHASE_CYCLE_SECONDS := 6.0

enum Mode { ZERO, POLE }

@onready var portrait: ColorRect = $Portrait
@onready var marker_layer: Control = $MarkerLayer
@onready var zero_button: Button = $Toolbar/ZeroButton
@onready var pole_button: Button = $Toolbar/PoleButton
@onready var undo_button: Button = $Toolbar/UndoButton
@onready var clear_button: Button = $Toolbar/ClearButton
@onready var pause_button: Button = $Toolbar/PauseButton
@onready var status_label: Label = $Toolbar/Status
@onready var portrait_material: ShaderMaterial = portrait.material as ShaderMaterial

var mode := Mode.ZERO
var zeros: Array[Vector2] = []
var poles: Array[Vector2] = []
var history: Array[int] = []
var phase := 0.0
var animate := true

func _ready() -> void:
	zero_button.pressed.connect(_select_zero)
	pole_button.pressed.connect(_select_pole)
	undo_button.pressed.connect(_undo)
	clear_button.pressed.connect(_clear)
	pause_button.pressed.connect(_toggle_animation)
	portrait.resized.connect(_geometry_changed)
	_update_mode_buttons()
	_sync_shader()
	_geometry_changed()

func _process(delta: float) -> void:
	if not animate:
		return
	phase = fmod(phase + TAU * delta / PHASE_CYCLE_SECONDS, TAU)
	portrait_material.set_shader_parameter("phase_offset", phase)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch and event.pressed:
		_place_from_screen(event.position)
	elif event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		_place_from_screen(event.position)

func _place_from_screen(screen_position: Vector2) -> void:
	var rect := portrait.get_global_rect()
	if not rect.has_point(screen_position):
		return

	if mode == Mode.ZERO and zeros.size() >= MAX_POINTS:
		return
	if mode == Mode.POLE and poles.size() >= MAX_POINTS:
		return

	var point := _screen_to_complex(screen_position)
	if mode == Mode.ZERO:
		zeros.append(point)
		history.append(Mode.ZERO)
	else:
		poles.append(point)
		history.append(Mode.POLE)

	_sync_shader()
	_rebuild_markers()

func _screen_to_complex(screen_position: Vector2) -> Vector2:
	var rect := portrait.get_global_rect()
	var uv := (screen_position - rect.position) / rect.size
	var aspect := rect.size.x / rect.size.y
	return Vector2(
		(uv.x * 2.0 - 1.0) * Y_HALF_EXTENT * aspect,
		(1.0 - uv.y * 2.0) * Y_HALF_EXTENT
	)

func _complex_to_marker_position(point: Vector2) -> Vector2:
	var size := portrait.size
	var aspect := size.x / size.y
	var x_half := Y_HALF_EXTENT * aspect
	var uv := Vector2(
		(point.x / x_half + 1.0) * 0.5,
		(1.0 - point.y / Y_HALF_EXTENT) * 0.5
	)
	return portrait.position + uv * size

func _packed(points: Array[Vector2]) -> PackedVector2Array:
	var result := PackedVector2Array()
	for index in MAX_POINTS:
		result.append(points[index] if index < points.size() else Vector2.ZERO)
	return result

func _sync_shader() -> void:
	portrait_material.set_shader_parameter("zero_count", zeros.size())
	portrait_material.set_shader_parameter("pole_count", poles.size())
	portrait_material.set_shader_parameter("zeros", _packed(zeros))
	portrait_material.set_shader_parameter("poles", _packed(poles))
	portrait_material.set_shader_parameter("phase_offset", phase)
	_update_status()

func _geometry_changed() -> void:
	if portrait.size.y <= 0.0:
		return
	portrait_material.set_shader_parameter("aspect", portrait.size.x / portrait.size.y)
	portrait_material.set_shader_parameter("y_half_extent", Y_HALF_EXTENT)
	_rebuild_markers()

func _rebuild_markers() -> void:
	for child in marker_layer.get_children():
		child.queue_free()

	for point in zeros:
		_add_marker(point, "○")
	for point in poles:
		_add_marker(point, "×")

func _add_marker(point: Vector2, glyph: String) -> void:
	var marker := Label.new()
	marker.text = glyph
	marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
	marker.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	marker.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	marker.add_theme_font_size_override("font_size", 34)
	marker.add_theme_color_override("font_color", Color.WHITE)
	marker.add_theme_color_override("font_outline_color", Color.BLACK)
	marker.add_theme_constant_override("outline_size", 5)
	marker.size = Vector2(48.0, 48.0)
	marker.position = _complex_to_marker_position(point) - marker.size * 0.5
	marker_layer.add_child(marker)

func _select_zero() -> void:
	mode = Mode.ZERO
	_update_mode_buttons()

func _select_pole() -> void:
	mode = Mode.POLE
	_update_mode_buttons()

func _update_mode_buttons() -> void:
	zero_button.set_pressed_no_signal(mode == Mode.ZERO)
	pole_button.set_pressed_no_signal(mode == Mode.POLE)

func _undo() -> void:
	if history.is_empty():
		return
	var last_mode := history.pop_back()
	if last_mode == Mode.ZERO:
		zeros.pop_back()
	else:
		poles.pop_back()
	_sync_shader()
	_rebuild_markers()

func _clear() -> void:
	zeros.clear()
	poles.clear()
	history.clear()
	_sync_shader()
	_rebuild_markers()

func _toggle_animation() -> void:
	animate = not animate
	pause_button.text = "pause" if animate else "play"

func _update_status() -> void:
	status_label.text = "○ %d    × %d" % [zeros.size(), poles.size()]
	undo_button.disabled = history.is_empty()
	clear_button.disabled = history.is_empty()
