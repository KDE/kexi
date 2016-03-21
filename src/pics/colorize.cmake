#
# Colorizes Kexi object icons for dark backgrounds
#

macro(_colorize _name _size _oldcolor _color)
    file(READ icons/breeze/actions/${_size}/${_name}.svg _SVG)
    string(REPLACE "fill:#${_oldcolor}" "fill:#${_color}" _NEW_SVG "${_SVG}")
    file(WRITE icons/breeze/actions/${_ICON_SIZE}/${_name}@dark.svg "${_NEW_SVG}")
endmacro()

foreach(_ICON_SIZE 16 22 32)
    _colorize("table" ${_ICON_SIZE} "2c3e50" "a0b5cb")
    _colorize("query" ${_ICON_SIZE} "2c3e50" "a0b5cb")
    _colorize("form" ${_ICON_SIZE} "2980b9" "39b0ff")
    _colorize("report" ${_ICON_SIZE} "27ae60" "26d26f")
    _colorize("macro" ${_ICON_SIZE} "8e44ad" "d164ff")
    _colorize("script" ${_ICON_SIZE} "d35400" "ff771c")
endforeach()
