extends Control

const MAX_POINTS := 16
const INITIAL_Y_HALF_EXTENT := 2.5
const MIN_Y_HALF_EXTENT := 0.05
const MAX_Y_HALF_EXTENT := 100.0
const ZOOM_FACTOR := 1.5
const PHASE_CYCLE_SECONDS := 6.0

enum Mode { MOVE, ZERO, POLE }

@onready var portrait: ColorRect = $Portrait
@onready var marker_layer: Control = $MarkerLayer
@onready var move_button: Button = $Toolbar/MoveButton
@onready var zero_button: Button = $Toolbar/ZeroButton
@onready var pole_button: Button = $Toolbar/PoleButton
@onready var zoom_out_button: Button = $Toolbar/ZoomOutButton
@onready var zoom_in_button: Button = $Toolbar/ZoomInButton
@onready var center_button: Button = $Toolbar/CenterButton
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
var view_center := Vector2.ZERO
var y_half_extent := INITIAL_Y_HALF_EXTENT

func _ready() -> void:
	move_button.pressed.connect(_select_move)
	zero_button.pressed.connect(_select_zero)
	pole_button.pressed.connect(_select_pole)
	zoom_out_button.pressed.connect(_zoom_out)
	zoom_in_button.pressed.connect(_zoom_in)
	center_button.pressed.connect(_center_view)
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
	if mode == Mode.MOVE:
		if event is InputEventScreenDrag and portrait.get_global_rect().has_point(event.position):
			_pan_pixels(event.relative)
		elif event is InputEventMouseMotion and (event.button_mask & MOUSE_BUTTON_MASK_LEFT) != 0 and portrait.get_global_rect().has_point(event.position):
			_pan_pixels(event.relative)
		return

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
	elif mode == Mode.POLE:
		poles.append(point)
		history.append(Mode.POLE)

	_sync_shader()
	_rebuild_markers()

func _screen_to_complex(screen_position: Vector2) -> Vector2:
	var rect := portrait.get_global_rect()
	var uv := (screen_position - rect.position) / rect.size
	var aspect := rect.size.x / rect.size.y
	return view_center + Vector2(
		(uv.x * 2.0 - 1.0) * y_half_extent * aspect,
		(1.0 - uv.y * 2.0) * y_half_extent
	)

func _complex_to_marker_position(point: Vector2) -> Vector2:
	var size := portrait.size
	var aspect := size.x / size.y
	var x_half := y_half_extent * aspect
	var relative := point - view_center
	var uv := Vector2(
		(relative.x / x_half + 1.0) * 0.5,
		(1.0 - relative.y / y_half_extent) * 0.5
	)
	return portrait.position + uv * size

func _pan_pixels(relative_pixels: Vector2) -> void:
	if portrait.size.x <= 0.0 or portrait.size.y <= 0.0:
		return
	var x_half := y_half_extent * portrait.size.x / portrait.size.y
	view_center += Vector2(
		-relative_pixels.x * (2.0 * x_half / portrait.size.x),
		relative_pixels.y * (2.0 * y_half_extent / portrait.size.y)
	)
	_sync_camera()

func _zoom(scale: float) -> void:
	y_half_extent = clamp(y_half_extent * scale, MIN_Y_HALF_EXTENT, MAX_Y_HALF_EXTENT)
	_sync_camera()

func _shader_points(points: Array[Vector2]) -> Array[Vector2]:
	var result: Array[Vector2] = []
	for index in range(MAX_POINTS):
		result.append(points[index] if index < points.size() else Vector2.ZERO)
	return result

func _sync_shader() -> void:
	portrait_material.set_shader_parameter("zero_count", zeros.size())
	portrait_material.set_shader_parameter("pole_count", poles.size())
	portrait_material.set_shader_parameter("zeros", _shader_points(zeros))
	portrait_material.set_shader_parameter("poles", _shader_points(poles))
	portrait_material.set_shader_parameter("phase_offset", phase)
	_sync_camera()
	_update_status()

func _sync_camera() -> void:
	if portrait.size.y <= 0.0:
		return
	portrait_material.set_shader_parameter("aspect", portrait.size.x / portrait.size.y)
	portrait_material.set_shader_parameter("view_center", view_center)
	portrait_material.set_shader_parameter("y_half_extent", y_half_extent)
	_rebuild_markers()
	_update_status()

func _geometry_changed() -> void:
	_sync_camera()

func _rebuild_markers() -> void:
	for child in marker_layer.get_children():
		child.queue_free()

	for point in zeros:
		_add_marker(point, "O")
	for point in poles:
		_add_marker(point, "X")

func _add_marker(point: Vector2, glyph: String) -> void:
	var marker_position := _complex_to_marker_position(point)
	var portrait_rect := Rect2(portrait.position, portrait.size)
	if not portrait_rect.has_point(marker_position):
		return

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
	marker.position = marker_position - marker.size * 0.5
	marker_layer.add_child(marker)

func _select_move() -> void:
	mode = Mode.MOVE
	_update_mode_buttons()

func _select_zero() -> void:
	mode = Mode.ZERO
	_update_mode_buttons()

func _select_pole() -> void:
	mode = Mode.POLE
	_update_mode_buttons()

func _update_mode_buttons() -> void:
	move_button.set_pressed_no_signal(mode == Mode.MOVE)
	zero_button.set_pressed_no_signal(mode == Mode.ZERO)
	pole_button.set_pressed_no_signal(mode == Mode.POLE)

func _zoom_out() -> void:
	_zoom(ZOOM_FACTOR)

func _zoom_in() -> void:
	_zoom(1.0 / ZOOM_FACTOR)

func _center_view() -> void:
	view_center = Vector2.ZERO
	_sync_camera()

func _undo() -> void:
	if history.is_empty():
		return
	var last_mode := history.pop_back()
	if last_mode == Mode.ZERO:
		zeros.pop_back()
	else:
		poles.pop_back()
	_sync_shader()

func _clear() -> void:
	zeros.clear()
	poles.clear()
	history.clear()
	_sync_shader()

func _toggle_animation() -> void:
	animate = not animate
	pause_button.text = "pause" if animate else "play"

func _update_status() -> void:
	if not is_instance_valid(status_label):
		return
	status_label.text = "O %d  X %d   center (%.2f, %.2f)   +/- %.2f" % [
		zeros.size(), poles.size(), view_center.x, view_center.y, y_half_extent
	]
	undo_button.disabled = history.is_empty()
	clear_button.disabled = history.is_empty()
