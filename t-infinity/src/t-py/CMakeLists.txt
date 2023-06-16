
set(PYTHON_PLUGINS
        __init__.py
        FluidSolver.py
        RefineMeshAdapter.py
        plugin_helpers.py
        )

install(FILES ${PYTHON_PLUGINS} DESTINATION python/infinity_plugins)
install(FILES setup.py DESTINATION python)
