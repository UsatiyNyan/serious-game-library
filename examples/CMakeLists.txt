sl_target_attach_directory(${PROJECT_NAME} shaders)
sl_target_attach_directory(${PROJECT_NAME} textures)

sl_add_example(${PROJECT_NAME} 00_blank_window)
sl_add_example(${PROJECT_NAME} 01_lighting)
sl_add_example(${PROJECT_NAME} 02_material)
sl_add_example(${PROJECT_NAME} 03_lightmap)
sl_add_example(${PROJECT_NAME} 04_many_lights)
sl_add_example(${PROJECT_NAME} 05_gfx_system)
sl_add_example(${PROJECT_NAME} 06_engine)
