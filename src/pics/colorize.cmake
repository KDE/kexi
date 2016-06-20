#
# Colorizes Kexi object icons for alternative backgrounds
#

include(kexi_breeze_files.cmake)

macro(_colorize _filepath _oldcolor _color _colorized_filepath)
    file(READ ${_filepath} _SVG)
    string(REPLACE "fill:#${_oldcolor}" "fill:#${_color}" _NEW_SVG "${_SVG}")
    string(REPLACE "color:#${_oldcolor};" "color:#${_color};" _NEW_SVG "${_NEW_SVG}")
    string(REPLACE ".svg" "@dark.svg" "${_colorized_filepath}" "${_filepath}")
    file(WRITE "${${_colorized_filepath}}" "${_NEW_SVG}")
    MESSAGE(STATUS "_filepath=${_filepath}")
    MESSAGE(STATUS "_colorized_filepath=${${_colorized_filepath}}")
endmacro()

macro(_colorize_list _filepath _oldcolor _color _colorized_files)
    _colorize(${_filepath} ${_oldcolor} ${_color} _colorized_filepath)
    list(APPEND "${_colorized_files}" "${_colorized_filepath}")
endmacro()

# Special cases
foreach(_ICON_SIZE 16 22 32)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/table.svg"  "2c3e50" "a0b5cb" _SVG_COLORIZED_OBJECT_FILES)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/query.svg"  "2c3e50" "a0b5cb" _SVG_COLORIZED_OBJECT_FILES)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/form.svg"   "2980b9" "39b0ff" _SVG_COLORIZED_OBJECT_FILES)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/report.svg" "27ae60" "26d26f" _SVG_COLORIZED_OBJECT_FILES)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/macro.svg"  "8e44ad" "d164ff" _SVG_COLORIZED_OBJECT_FILES)
    _colorize_list("icons/breeze/actions/${_ICON_SIZE}/script.svg" "d35400" "ff771c" _SVG_COLORIZED_OBJECT_FILES)
endforeach()

set(ICONGREY_COLOR "4d4d4d")

foreach(_SVG_FILE ${_SVG_FILES})
    _colorize_list("${_SVG_FILE}" ${ICONGREY_COLOR} "7f8c8d" _SVG_COLORIZED_FILES)
endforeach()

#message(STATUS "_SVG_FILES=${_SVG_FILES}")
#message(STATUS "_SVG_COLORIZED_OBJECT_FILES=${_SVG_COLORIZED_OBJECT_FILES}")
#message(STATUS "_SVG_COLORIZED_FILES=${_SVG_COLORIZED_FILES}")
