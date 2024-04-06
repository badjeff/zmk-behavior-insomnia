zephyr_library()

zephyr_library_sources(src/behavior_insomnia_timer.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_INSOMNIA src/behavior_insomnia.c)

zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
