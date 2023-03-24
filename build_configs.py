release = ["-DCMAKE_BUILD_TYPE=Release"]
debug = ["-DCMAKE_BUILD_TYPE=Debug"]
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

IPC23_AGILE = [
    "DISJUNCTIVE_ACTION_LANDMARKS",
    # "PLUGIN_EAGER_GREEDY",
    "PLUGIN_LAZY_GREEDY",
]

IPC23_SATISFICING = [
    "ITERATED_SEARCH",
    "DISJUNCTIVE_ACTION_LANDMARKS",
    "PLUGIN_LAZY_GREEDY",
    "PLUGIN_LAZY_WASTAR",
]

IPC23_OPTIMAL = [
    "CYCLIC_LANDMARK_HEURISTIC",
    "DISJUNCTIVE_ACTION_LANDMARKS",
    "PLUGIN_ASTAR",
]

def get_build_config(plugins, debug=False, lp=False):
    config = ["-DDISABLE_PLUGINS_BY_DEFAULT=YES"] + [
        f"-DPLUGIN_{plugin}_ENABLED=True" for plugin in plugins
    ]
    if debug:
        config = ["-DCMAKE_BUILD_TYPE=Debug"] + config
    else:
        config = ["-DCMAKE_BUILD_TYPE=Release"] + config

    if not lp:
        config += ["-DUSE_LP=NO"]

    return config


ipc23_agl = get_build_config(IPC23_AGILE)
ipc23_agl_debug = get_build_config(IPC23_AGILE, debug=True)
ipc23_sat = get_build_config(IPC23_SATISFICING)
ipc23_sat_debug = get_build_config(IPC23_SATISFICING, debug=True)
ipc23_opt = get_build_config(IPC23_OPTIMAL, lp=True)
ipc23_opt_debug = get_build_config(IPC23_OPTIMAL, lp=True)

DEFAULT = "release"
DEBUG = "debug"
