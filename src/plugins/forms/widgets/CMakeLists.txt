set(KEXI_FORM_WIDGETS_PLUGIN_INSTALL_DIR ${KEXI_PLUGIN_INSTALL_DIR}/forms/widgets)

# Convenience macro that adds icon resource for given plugin and theme.
# Icons are placed in a path that contains _plugin_target.
# For example,  icons/org.kexi-project.form.widgets.web-browser/breeze/actions/16/kexiform-web-browser.svg.
# This helps to make plugins not conflicting.
macro(kexi_add_plugin_icons_rcc_file _plugin_target _theme)
    kexi_add_icons_rcc_file(${_plugin_target}_${_theme} ${_plugin_target} ${_theme} ${_plugin_target})
endmacro()

# the main widgets plugin
add_subdirectory(main)

if(MARBLE_FOUND)
#TODO  add_subdirectory(mapbrowser)
endif()

if(Qt5WebKitWidgets_FOUND)
  add_subdirectory(webbrowser)
endif()
