release = ["-DCMAKE_BUILD_TYPE=Release"]
debug = ["-DCMAKE_BUILD_TYPE=Debug"]
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

IPC23_PLUGINS = [
    "PLUGIN_ASTAR",
    "LANDMARKS",
    "PLUGIN_LAZY_GREEDY",
]
IPC23_STANDARD = ["-DDISABLE_PLUGINS_BY_DEFAULT=YES"] + [
    f"-DPLUGIN_{plugin}_ENABLED=True" for plugin in IPC23_PLUGINS
]
ipc23 = ["-DCMAKE_BUILD_TYPE=Release"] + IPC23_STANDARD
ipc23_debug = ["-DCMAKE_BUILD_TYPE=Debug"] + IPC23_STANDARD
ipc23_no_lp = ipc23 + ["-DUSE_LP=NO"]
ipc23_no_lp_debug = ipc23_debug + ["-DUSE_LP=NO"]

ipc23_eager = ipc23 + ["-DPLUGIN_PLUGIN_EAGER_GREEDY_ENABLED=True"]

DEFAULT = "ipc23"
DEBUG = "ipc23_debug"
