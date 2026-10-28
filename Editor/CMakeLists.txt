add_executable(KizuriEditor src/main.cpp src/EditorApp.cpp src/EditorPanels.cpp src/EditorTheme.cpp src/FileDialog.cpp)
set_target_properties(KizuriEditor PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(KizuriEditor PRIVATE KizuriCore KizuriRender KizuriPhysics KizuriAnimation KizuriAudio KizuriScripting KizuriScene KizuriAssets KizuriGame comdlg32 shell32 user32)
add_custom_command(TARGET KizuriEditor POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:KizuriEditor>/Shaders COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Samples/Assets $<TARGET_FILE_DIR:KizuriEditor>/Assets)
