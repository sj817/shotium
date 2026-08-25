{
  # How the addon is built. It is not built by `npm install`: shot itself is a
  # Chromium fork that takes hours and a checkout to compile, so the library
  # this links against is a release artifact, and the addon is built against it
  # once per platform and shipped prebuilt.
  #
  #   SHOT_INCLUDE_DIR=/path/to/shot   (the directory holding shot_api.h)
  #   SHOT_LIB_DIR=/path/to/out/Shot   (the directory holding the library)
  #   npx node-gyp rebuild
  "variables": {
    "shot_include_dir%": "<!(node -p \"process.env.SHOT_INCLUDE_DIR || require('path').resolve('../../shot')\")",
    "shot_lib_dir%": "<!(node -p \"process.env.SHOT_LIB_DIR || require('path').resolve('../../out/Shot')\")"
  },
  "targets": [
    {
      "target_name": "shotium",
      "sources": ["binding.cc"],
      "include_dirs": ["<(shot_include_dir)"],
      # Node-API 8 is Node 12.22 / 14.17 and up. Declaring it pins the surface
      # this addon may use, so a build cannot quietly start depending on a
      # newer node than the one it claims to support.
      "defines": ["NAPI_VERSION=8"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "conditions": [
        ["OS=='win'", {
          # The import library GN writes beside the DLL. The DLL itself is
          # found at run time in the directory the .node was loaded from.
          "libraries": ["<(shot_lib_dir)/shot.dll.lib"]
        }],
        ["OS=='linux'", {
          "libraries": [
            "-L<(shot_lib_dir)",
            "-lshot",
            # $ORIGIN, escaped past make: the library ships beside the .node,
            # not in a system directory, and nothing should be searching the
            # host's library path for something named this generally.
            "-Wl,-rpath,'$$ORIGIN'"
          ]
        }],
        ["OS=='mac'", {
          "libraries": ["-L<(shot_lib_dir)", "-lshot", "-Wl,-rpath,@loader_path"]
        }]
      ]
    }
  ]
}
