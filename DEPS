# This file is used to manage the dependencies of the Chromium src repo. It is
# used by gclient to determine what version of each dependency to check out, and
# where.
#
# For more information, please refer to the official documentation:
#   https://sites.google.com/a/chromium.org/dev/developers/how-tos/get-the-code
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.
#
# -----------------------------------------------------------------------------
# Rolling deps
# -----------------------------------------------------------------------------
# All repositories in this file are git-based, using Chromium git mirrors where
# necessary (e.g., a git mirror is used when the source project is SVN-based).
# To update the revision that Chromium pulls for a given dependency:
#
#  # Create and switch to a new branch
#  git new-branch depsroll
#  # Run roll-dep (provided by depot_tools) giving the dep's path and optionally
#  # a regex that will match the line in this file that contains the current
#  # revision. The script ALWAYS rolls the dependency to the latest revision
#  # in origin/master. The path for the dep should start with src/.
#  roll-dep src/third_party/foo_package/src foo_package.git
#  # You should now have a modified DEPS file; commit and upload as normal
#  git commit -a
#  git cl upload
#
# For more on the syntax and semantics of this file, see:
#   https://bit.ly/chromium-gclient-conditionals
#
# which is a bit incomplete but the best documentation we have at the
# moment.

# We expect all git dependencies specified in this file to be in sync with git
# submodules (gitlinks).
git_dependencies = 'SYNC'

gclient_gn_args_file = 'src/build/config/gclient_args.gni'
gclient_gn_args = [
  'android_ndk_version',
  'build_with_chromium',
  'checkout_android',
  'checkout_android_prebuilts_build_tools',
  'checkout_clang_coverage_tools',
  'checkout_copybara',
  'checkout_fuchsia',
  'checkout_glic_e2e_tests',
  'checkout_ios_webkit',
  'checkout_openxr',
  'checkout_src_internal',
  'cros_boards',
  'cros_boards_with_qemu_images',
  'generate_location_tags',
]


vars = {
  # The version of the NDK. Set here, to allow the autoroller to update this
  # value when updating the CIPD hash.
  'android_ndk_version': Str('2@30.0.16138531'),

  # Variable that can be used to support multiple build scenarios, like having
  # Chromium specific targets in a client project's GN file or sync dependencies
  # conditionally etc.
  'build_with_chromium': True,

  # By default, we should check out everything needed to run on the main
  # chromium waterfalls. This var can be also be set to "small", in order
  # to skip things are not strictly needed to build chromium for development
  # purposes, by adding the following line to src.git's .gclient entry:
  #      "custom_vars": { "checkout_configuration": "small" },
  'checkout_configuration': 'default',

  # By default, don't check out android. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the problem in gclient is fixed.
  'checkout_android': False,

  # By default, don't check out Fuchsia. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the problem in gclient is fixed.
  'checkout_fuchsia': False,

  # For code related to internal Fuchsia images.
  'checkout_fuchsia_internal': False,

  # Fetches the internal Fuchsia SDK boot images, with the images in a
  # comma-separated list.
  'checkout_fuchsia_internal_images': '',

  # Used for downloading the Fuchsia SDK without running hooks.
  'checkout_fuchsia_no_hooks': False,

  # Pull in Android prebuilts build tools so we can create Java xrefs
  'checkout_android_prebuilts_build_tools': False,

  # By default, do not check out Cast3P.
  'checkout_cast3p': False,

  # By default, do not check out Chromium autofill captured sites test
  # dependencies. These dependencies include very large numbers of very
  # large web capture files. Captured sites test dependencies are also
  # restricted to Googlers only.
  'checkout_chromium_autofill_test_dependencies': False,

  # By default, do not check out Chromium password manager captured sites test
  # dependencies. These dependencies include very large numbers of very
  # large web capture files. Captured sites test dependencies are also
  # restricted to Googlers only.
  'checkout_chromium_password_manager_test_dependencies': False,

  # By default, checkout JavaScript coverage node modules. These packages
  # are used to post-process raw v8 coverage reports into IstanbulJS compliant
  # output.
  'checkout_js_coverage_modules': True,

  # By default, do not check out src-internal. This can be overridden e.g. with
  # custom_vars.
  'checkout_src_internal': False,

  # Checkout legacy src_internal. This variable is ignored if
  # checkout_src_internal is set as false.
  'checkout_legacy_src_internal': True,


  # Checkout test code and archives for glic E2E tests.
  'checkout_glic_e2e_tests': False,

  # For super-internal deps. Set by the official builders.
  'checkout_google_internal': False,

  # Checkout SODA (Speech On-Device API go/chrome-live-caption)
  'checkout_soda': False,

  # Fetch the additional packages and files needed to run all of the
  # telemetry tests. This is false by default as some stuff is only
  # privately accessible.
  'checkout_telemetry_dependencies': False,

  # Bots that don't consume WPR archives can skip downloading
  # them.
  'skip_wpr_archives_download': False,

  # Fetch the prebuilt binaries for llvm-cov and llvm-profdata. Needed to
  # process the raw profiles produced by instrumented targets (built with
  # the gn arg 'use_clang_coverage').
  'checkout_clang_coverage_tools': False,

  # Fetch the pgo profiles to optimize official builds.
  'checkout_pgo_profiles': False,

  # Fetch clang-tidy into the same bin/ directory as our clang binary.
  'checkout_clang_tidy': False,

  # Fetch clangd into the same bin/ directory as our clang binary.
  'checkout_clangd': False,

  # Fetch prebuilt and prepackaged Bazelisk tool/executable. Bazelisk is currently
  # only needed by `chromium/src/tools/rust/build_crubit.py` when building and
  # packaging `//third_party/rust-toolchain` - it is *not* needed during regular
  # Chromium builds.
  'checkout_bazelisk': False,
  'bazelisk_version': 'version:3@1.29.0',

  # By default checkout the OpenXR loader library on Windows, Linux and Android.
  # The OpenXR backend for VR in Chromium is supported on these platforms;
  # support for other platforms may be added in the future.
  'checkout_openxr' : 'checkout_win or checkout_linux or checkout_android',

  # By default, do not check out instrumented libraries. These prebuilt
  # binaries are only consumed by MSan builds (`is_msan = true` in GN).
  #
  # They should only be checked out on Linux environments with a full checkout
  # configuration (`checkout_linux and checkout_configuration != "small"`)
  # when specifically compiling MSan targets.
  'checkout_instrumented_libraries': False,

  # By default bot checkouts the WPR archive files only when this
  # flag is set True.
  'checkout_wpr_archives': False,

  # By default, do not check out WebKit for iOS, as it is not needed unless
  # running against ToT WebKit rather than system WebKit. This can be overridden
  # e.g. with custom_vars.
  'checkout_ios_webkit': False,

  # Fetches only the SDK boot images that match at least one of the
  # entries in a comma-separated list.
  #
  # Available images:
  #   Emulation:
  #   - core.x64-dfv2
  #   - terminal.x64
  #   - terminal.qemu-arm64
  #   - workstation.qemu-x64
  #   Hardware:
  #   - workstation_eng.chromebook-x64
  #   - workstation_eng.chromebook-x64-dfv2
  #
  # Since the images are hundreds of MB, default to only downloading the image
  # most commonly useful for developers. Bots and developers that need to use
  # other images can override this with additional images.
  'checkout_fuchsia_boot_images': "terminal.x64",
  'checkout_fuchsia_product_bundles': '"{checkout_fuchsia_boot_images}" != ""',

  # By default, do not check out files required to run fuchsia tests in
  # qemu on linux-arm64 machines.
  'checkout_fuchsia_for_arm64_host': False,

  # By default, download the fuchsia sdk from the public sdk directory.
  'fuchsia_sdk_cipd_prefix': 'fuchsia/sdk/core/',

  # By default, download the fuchsia images from the fuchsia GCS bucket.
  'fuchsia_images_bucket': 'fuchsia',

  # Default to the empty board. Desktop Chrome OS builds don't need cros SDK
  # dependencies. Other Chrome OS builds should always define this explicitly.
  'cros_boards': Str(''),
  'cros_boards_with_qemu_images': Str(''),
  # Building for CrOS is only supported on linux currently.
  'checkout_simplechrome': '"{cros_boards}" != ""',
  'checkout_simplechrome_with_vms': '"{cros_boards_with_qemu_images}" != ""',

  # Generate location tag metadata to include in tests result data uploaded
  # to ResultDB. This isn't needed on some configs and the tool that generates
  # the data may not run on them, so we make it possible for this to be
  # turned off. Note that you also generate the metadata but not include it
  # via a GN build arg (tests_have_location_tags).
  'generate_location_tags': True,

  # By default, do not check out Copybara 3pp dependency that is specifically
  # needed by Cronet gn2bp CI builder.
  'checkout_copybara': False,

  # By default, check out the latest press benchmark versions for local
  # testing and debugging. Benchmarks are also hosted on publicly accessible
  # sites as backup solution.
  'checkout_press_benchmarks': 'checkout_configuration != "small"',

  # luci-go CIPD package version.
  # Make sure the revision is uploaded by infra-packagers builder.
  # https://ci.chromium.org/p/infra-internal/g/infra-packagers/console
  'luci_go': 'git_revision:3d1174859c0a5bb00d87c78506f8c031afad16aa',

  # This can be overridden, e.g. with custom_vars, to build clang from HEAD
  # instead of downloading the prebuilt pinned revision.
  'llvm_force_head_revision': False,

  # This can be overridden, e.g. with custom_vars, to build rust from HEAD
  # against ToT LLVM, instead of downloading the prebuilt pinned revision.
  'rust_force_head_revision': False,

  # Make Dawn skip its standalone dependencies
  'dawn_standalone': False,

  # Fetch configuration files required for the 'use_remoteexec' gn arg
  'download_remoteexec_cfg': False,
  # RBE instance to use for running remote builds
  # Ignored if reapi_instance is configured for non-RBE address.
  'rbe_instance': Str('projects/rbe-chrome-untrusted/instances/default_instance'),
  # REAPI instance for non-RBE backends.
  # need to set reapi_address too.
  'reapi_instance': Str(''),
  # REAPI address for REAPI backends.
  'reapi_address': Str(''),
  # REAPI backend config path for Siso.
  # pathname relative to build/config/siso/backend_config, or absolute path.
  'reapi_backend_config_path': Str(''),
  # REAPI credential helper to use for Siso.
  # binary available on `PATH`, or absolute path.
  'reapi_credential_helper': Str(''),
  # siso CIPD package version.
  'siso_version': 'git_revision:2f0eb0113740f469481a64760ab8feabe529a5b1',

  # CPython 3 CIPD package version for Siso hermetic toolchain.
  'cpython3_version': 'version:3@3.11.9.chromium.38',

  # reclient options.
  # download reclient binaries, required for 'use_reclient` gn arg.
  # TODO(crbug.com/448517720): make it false by default.
  'download_reclient': 'checkout_chromeos',
  # RBE project to download rewrapper config files for. Only needed if
  # different from the project used in 'rbe_instance'
  'rewrapper_cfg_project': Str(''),
  # reclient CIPD package
  'reclient_package': 'infra/rbe/client/',
  # reclient CIPD package version
  'reclient_version': 're_client_version:0.185.0.db415f21-gomaip',

  # screen-ai CIPD packages
  'screen_ai_linux': 'version:153.01',
  'screen_ai_macos_amd64': 'version:153.01',
  'screen_ai_macos_arm64': 'version:153.01',
  'screen_ai_windows_amd64': 'version:153.01',
  'screen_ai_windows_386': 'version:153.01',

  # download libaom test data
  'download_libaom_testdata': False,

  # download libvpx test data
  'download_libvpx_testdata': False,

  'android_git': 'https://android.googlesource.com',
  'aomedia_git': 'https://aomedia.googlesource.com',
  'boringssl_git': 'https://boringssl.googlesource.com',
  'chrome_git': 'https://chrome-internal.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',
  'dawn_git': 'https://dawn.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'quiche_git': 'https://quiche.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',
  'swiftshader_git': 'https://swiftshader.googlesource.com',
  'webrtc_git': 'https://webrtc.googlesource.com',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling V8
  # and whatever else without interference from each other.
  'src_internal_revision': '6ea9910ef44323b4d2ce3ea77fee2d6adcbcda23',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling V8
  # and whatever else without interference from each other.
  'v8_revision': 'c0305e3e1264c7c9bb6edf1288b6c03cd72b2c3d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ANGLE
  # and whatever else without interference from each other.
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling SwiftShader
  # and whatever else without interference from each other.
  'swiftshader_revision': '5b0479bd2d15058aaa9eb490e364f920ff824a8c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling PDFium
  # and whatever else without interference from each other.
  'pdfium_revision': '482ed62233d7fde7e3bd35a26a0af26f03e228b9',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling BoringSSL
  # and whatever else without interference from each other.
  'boringssl_revision': 'f00ee8085f98bcca5991926ea0c6383068ee6d2a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Fuchsia sdk
  # and whatever else without interference from each other.
  'fuchsia_version': 'version:33.20260903.4.1',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling googletest
  # and whatever else without interference from each other.
  'googletest_revision': '4fe3307fb2d9f86d19777c7eb0e4809e9694dde7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling lss
  # and whatever else without interference from each other.
  'lss_revision': '29164a80da4d41134950d76d55199ea33fbb9613',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling breakpad
  # and whatever else without interference from each other.
  'breakpad_revision': 'a9df3bd99cf8b863a92502ef906b5ddc399c2c9a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling freetype
  # and whatever else without interference from each other.
  'freetype_revision': '5c79d6cd1ac73d70a55f3d963fb568aa32f6d794',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling HarfBuzz
  # and whatever else without interference from each other.
  'harfbuzz_revision': '46ea946ab4ad4d01fd83df7a6add740fd2590a73',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Emoji Segmenter
  # and whatever else without interference from each other.
  'emoji_segmenter_revision': '955936be8b391e00835257059607d7c5b72ce744',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling OTS
  # and whatever else without interference from each other.
  'ots_revision': '46bea9879127d0ff1c6601b078e2ce98e83fcd33',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': 'bc8f307a4d44148a31e34ab48c77d0834a404d20',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling CrossBench
  # and whatever else without interference from each other.
  'crossbench_revision': 'e64dab2ad45e3ddd838cbf02c6a9eed0c3c3c46b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling CrossBench
  # and whatever else without interference from each other.
  'crossbench_web_tests_revision': '28ae53ad844e08c456d9f448f122a3fffbb594f2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libFuzzer
  # and whatever else without interference from each other.
  'libfuzzer_revision': '8c09a8f461575f5fc009c4053a18611e23e3879c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fuzztest
  # and whatever else without interference from each other.
  'fuzztest_revision': '25f6bff894a558fd6b7076a22f9e881d807304e6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling domato
  # and whatever else without interference from each other.
  'domato_revision': 'fadff396cc45d521cc594d3e2396e27e887b1963',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling devtools-frontend
  # and whatever else without interference from each other.
  'devtools_frontend_revision': '66cbff3fcf324246508e64bc8c1e4ad83ed147b3',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libprotobuf-mutator
  # and whatever else without interference from each other.
  'libprotobuf-mutator': 'c1c950eae0440c3808f2b8bd7c57d0c6a42c1a90',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_sdk_build-tools_version
  # and whatever else without interference from each other.
  'android_sdk_build-tools_version': 'version_37.0.0',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_sdk_emulator_version
  # and whatever else without interference from each other.
  'android_sdk_emulator_version': '9lGp8nTUCRRWGMnI_96HcKfzjnxEJKUcfvfwmA3wXNkC',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_sdk_platform-tools_version
  # and whatever else without interference from each other.
  'android_sdk_platform-tools_version': 'qTD9QdBlBf3dyHsN1lJ0RH6AhHxR42Hmg2Ih-Vj4zIEC',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_sdk_platforms_version
  # and whatever else without interference from each other.
  'android_sdk_platforms_version': 'WhtP32Q46ZHdTmgCgdauM3ws_H9iPoGKEZ_cPggcQ6wC',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'dawn_revision': 'a4d741cf146de8c9b55f7e7ab67059ffaf710f2b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'quiche_revision': 'efdf29da0168bc8277df5d94419c8cc04876581b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ink
  # and whatever else without interference from each other.
  'ink_revision': '4aa45f40c047ee173dcbf1095ba9353e3308709d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ios_webkit
  # and whatever else without interference from each other.
  'ios_webkit_revision': 'f8c0fe750d94b7db23d193c0b1f31858c2537620',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libexpat
  # and whatever else without interference from each other.
  'libexpat_revision': 'a851869e111c06fbcdc89edf639abf5fc2b6deba',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jetstream-main
  # and whatever else without interference from each other.
  'jetstream_main_revision': 'c603c04db8505477867974a69789309ded2cc948',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jetstream-v2.2
  # and whatever else without interference from each other.
  'jetstream_2.2_revision': '2145cedef4ca2777b792cb0059d3400ee2a6153c',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jetstream-v3.0
  # and whatever else without interference from each other.
  'jetstream_3.0_revision': '06785cf861ac44855f168cbbe829278c2802e6de',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling wuffs
  # and whatever else without interference from each other.
  'wuffs_revision': '7411f488fe2e2c205c3d3b3d28638b7356522930',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling crabbyavif
  # and whatever else without interference from each other.
  'crabbyavif_revision': '3aaf0add212aea38121ccf5b247b8c790ae8a13a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Speedometer main
  # and whatever else without interference from each other.
  'speedometer_main_revision': 'f866d2b077038bc85bea5edb7bff8e05cadef0b2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Speedometer v3.1
  # and whatever else without interference from each other.
  'speedometer_3.1_revision': '1386415be8fef2f6b6bbdbe1828872471c5d802a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Speedometer v3.0
  # and whatever else without interference from each other.
  'speedometer_3.0_revision': '8d67f28d0281ac4330f283495b7f48286654ad7d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Speedometer v2.1
  # and whatever else without interference from each other.
  'speedometer_2.1_revision': '8bf7946e39e47c875c00767177197aea5727e84a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Speedometer v2.0
  # and whatever else without interference from each other.
  'speedometer_2.0_revision': '732af0dfe867f8815e662ac637357e55f285dbbb',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling nearby
  # and whatever else without interference from each other.
  'nearby_revision': '0bad8b0c9877f92eeeb550654f1ea51a71a085e4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling securemessage
  # and whatever else without interference from each other.
  'securemessage_revision': 'fa07beb12babc3b25e0c5b1f38c16aa8cb6b8f84',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ukey2
  # and whatever else without interference from each other.
  'ukey2_revision': '0275885d8e6038c39b8a8ca55e75d1d4d1727f47',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'cros_components_revision': '0abb2efaa3d16db861c9710b193c39e657ac3bdf',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'result_adapter_revision': 'git_revision:5fb3ca203842fd691cab615453f8e5a14302a1d8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'libcxxabi_revision':    'fc1897a2c12aa27e703c3ed48b62eba8abf4ce19',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'libunwind_revision':    'a8be4de7dec999dafd1857f3516a7f87ffbe0b22',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'clang_format_revision':    '70510081984cfcdb14a15b3e08dfe9776dc7ed37',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling feed
  # and whatever else without interference from each other.
  'highway_revision': '2607d3b5b0113992fe84d3848859eae13b3b52c1',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling ffmpeg
  # and whatever else without interference from each other.
  'ffmpeg_revision': 'ba5e7eaea38b6a54157870e0085b2e8c75aa6bcb',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling webpagereplay
  # and whatever else without interference from each other.
  'webpagereplay_revision': 'b2b856131e36c99e9de9c419fe8ca02f857082ba',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision':    '4c5b71179128c90a696bcdd11ffd4c67bad0e0de',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'compiler_rt_revision': '18a0a1d971961fce19a3568c33b0b0b2c23fc098',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clusterfuzz-data
  # and whatever else without interference from each other.
  'clusterfuzz_data_revision':'b36441669795caeafa2557a0e91ad72628b33c85',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling agents-internal
  # and whatever else without interference from each other.
  'agents_internal_revision': 'fbd26bb4531eaaecfc8cb80a6eab3c85a6e672dd',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling agents-public
  # and whatever else without interference from each other.
  'agents_public_revision': '73470db4a7810560ec92e8aa27ff3c6a8a36bea5',

  # If you change this, also update the libc++ revision in
  # //buildtools/deps_revisions.gni.
  'libcxx_revision':       '97b436da4c33663581d394f4ee0a5977fc38c2f4',

  # GN CIPD package version.
  'gn_version': 'git_revision:5df47e556efde72cc576d8a3af23b58b614345bd',

  # ninja CIPD package.
  'ninja_package': 'infra/3pp/tools/ninja/',

  # ninja CIPD package version.
  # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
  'ninja_version': 'version:3@1.12.1.chromium.4',

  # 'magic' variable to tell depot_tools that git submodules should be accepted
  # but parity with DEPS file is expected.
  'SUBMODULE_MIGRATION': 'True',

  # condition to allowlist deps to be synced in Cider. Allowlisting is needed
  # because not all deps are compatible with Cider. Once we migrate everything
  # to be compatible we can get rid of this allowlisting mecahnism and remove
  # this condition. Tracking bug for removing this condition: b/349365433
  'non_git_source': 'True',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, contact chrome infrastracture team.
allowed_hosts = [
  'android.googlesource.com',
  'aomedia.googlesource.com',
  'boringssl.googlesource.com',
  'chrome-infra-packages.appspot.com',
  'chrome-internal.googlesource.com',
  'chromium.googlesource.com',
  'dawn.googlesource.com',
  'pdfium.googlesource.com',
  'quiche.googlesource.com',
  'skia.googlesource.com',
  'swiftshader.googlesource.com',
  'webrtc.googlesource.com',

   # TODO(337061377): Move into a separate allowed gcs bucket list.
  'chromium-ads-detection',
  'chromium-browser-clang',
  'chromium-clang-format',
  'chromium-doclava',
  'chromium-nodejs',
  'chrome-linux-sysroot',
  'chromium-fonts',
  'chromium-style-perftest',
  'chromium-telemetry',
  'chromium-webrtc-resources',
  'meet-bundles',
  'perfetto',
]

deps = {
  'src/build/linux/debian_bullseye_amd64-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_x64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1770327973518330,
        'object_name': '52d61d4446ffebfaa3dda2cd02da4ab4876ff237853f46d273e7f9b666652e1d',
        'sha256sum': '52d61d4446ffebfaa3dda2cd02da4ab4876ff237853f46d273e7f9b666652e1d',
        'size_bytes': 19727236,
      },
    ],
  },
  'src/build/linux/debian_bullseye_arm64-sysroot': {
    'bucket': 'chrome-linux-sysroot',
    'condition': 'checkout_linux and checkout_arm64 and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'generation': 1770327978874031,
        'object_name': 'c7176a4c7aacbf46bda58a029f39f79a68008d3dee6518f154dcf5161a5486d8',
        'sha256sum': 'c7176a4c7aacbf46bda58a029f39f79a68008d3dee6518f154dcf5161a5486d8',
        'size_bytes': 18420984,
      },
    ],
  },
  'src/buildtools/win-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "win" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': 'fb0bcaf406ad6f0bd6b4a6347d2abe78a94fb13e',
        'sha256sum': 'ee7a1f89733de23daf8ee035f8aebafa52410c9f4dd1f05ee12a95ce517a0953',
        'size_bytes': 3799552,
        'generation': 1779297371701340,
        'output_file': 'clang-format.exe',
      },
    ],
  },
  'src/buildtools/mac-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "mac" and host_cpu == "x64" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '35ddc571ab654a4f58d7b35f394b04835d0b98f9',
        'sha256sum': '39a9c4e138a8c5e6ab5cd7b21321c2ca03f3f518a6ed07de82162d9f4f37318f',
        'size_bytes': 3567528,
        'generation': 1779297382681217,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/buildtools/mac_arm64-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "mac" and host_cpu == "arm64" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': '945e45cbcb5443c3e569d5e0783fd012154fc4a7',
        'sha256sum': 'a1fb17260eda2971f52ed341501b93703b0fce69ffb43a179b891b66f6f63e5a',
        'size_bytes': 3245456,
        'generation': 1779297393605542,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/buildtools/linux64-format': {
    'bucket': 'chromium-clang-format',
    'condition': 'host_os == "linux" and non_git_source',
    'dep_type': 'gcs',
    'objects': [
      {
        'object_name': 'f5485c39451137ee943021a0fb63738b306c5026',
        'sha256sum': '5d384f2a3e72f01c5c61dd07e3a2577723cfa20c8c019131467f00fb6da8a75c',
        'size_bytes': 3764032,
        'generation': 1779297360709713,
        'output_file': 'clang-format',
      },
    ],
  },
  'src/third_party/llvm-build/Release+Asserts': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'condition': 'not llvm_force_head_revision',
    'objects': [
      {
        'object_name': 'Linux_x64/clang-android-runtime-library-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'a1d8a22fd65d8d9a7c78dec60ef1506de0d7dc7934dcaeaa79ad4aaf96f4990e',
        'size_bytes': 6203844,
        'generation': 1788861327141103,
        'condition': 'checkout_android and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'affe3e9cd43989090aadb5e06bab304a9e31f01de05e61739f7b70753049588c',
        'size_bytes': 57245392,
        'generation': 1788861304062048,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-tidy-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'aca80d2eb2ed2603e7cda25dcb8fabfb5968a1915e9616d55868c380bbd9b6c5',
        'size_bytes': 14932308,
        'generation': 1788861304404783,
        'condition': 'host_os == "linux" and checkout_clang_tidy and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clangd-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'f7d9f5a5d4916b3d2f80623b58a4acbfd50e195c3fd7ef61f3e45160e464b611',
        'size_bytes': 15083300,
        'generation': 1788861304929437,
        'condition': 'host_os == "linux" and checkout_clangd and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvm-code-coverage-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '27403412c86f3a653bf2d709554f7e7fede4c4db56ef8f05266f9b432bacc837',
        'size_bytes': 2371408,
        'generation': 1788861305541490,
        'condition': 'host_os == "linux" and checkout_clang_coverage_tools and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '26c1f89a8138c1791f52be96cba3f81ec06fd44743f4688e99b3a0694f0e2748',
        'size_bytes': 5917380,
        'generation': 1788861304846854,
        'condition': '((checkout_linux or checkout_mac or checkout_android) and host_os == "linux") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '9e3069316bddcc559d9ea86f8ecabed5fdc8afa45c8641ec2f1561f4a9dce534',
        'size_bytes': 56702268,
        'generation': 1788861328955264,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '8c34155ab4c88076b78721754cae9084258dd8c4bb012334c94dbb8ba075d464',
        'size_bytes': 981308,
        'generation': 1788861348992469,
        'condition': 'checkout_mac and not host_os == "mac"',
      },
      {
        'object_name': 'Mac/clang-tidy-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'd2f8a20a5813cf7a646b9d3f67101c3bd23d65d4c0176c40f9ecc1010767a100',
        'size_bytes': 14929588,
        'generation': 1788861329319576,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac/clangd-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '1b8f988507971a8f7b63810d1c69df06fcfb2aeb6fd7a3125a4f5f4b5617ff38',
        'size_bytes': 16810624,
        'generation': 1788861330106473,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clangd',
      },
      {
        'object_name': 'Mac/llvm-code-coverage-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '1f8ee83c7ef57c75234c023cf032657ac3aa10b300ff4da210304051d36397d8',
        'size_bytes': 2419692,
        'generation': 1788861330064459,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac/llvmobjdump-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '1e60a0f90dc0d841a29dbfd6ff7b6036fe9cb06de8cc6bbd7b6d5a78417e1214',
        'size_bytes': 5903104,
        'generation': 1788861329796965,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '56e00034a68d5e8d939febafc1cd3677b1347741a033cb1c3e45badfceb024a1',
        'size_bytes': 47554840,
        'generation': 1788861352420476,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/clang-tidy-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'b098a3339fc4d205246ea6d1c38898c26d4619ba6750b16121e483882330550f',
        'size_bytes': 12999564,
        'generation': 1788861351876696,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac_arm64/clangd-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '8740734687ac743b4f617477fa61c548b71d917c283329b602f1d5ac1e40066c',
        'size_bytes': 13317820,
        'generation': 1788861352162379,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clangd',
      },
      {
        'object_name': 'Mac_arm64/llvm-code-coverage-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'af635754ae9b10598dadc1e41649641c0c362977ab690ba102b9e05a6babd66d',
        'size_bytes': 2043356,
        'generation': 1788861353096893,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac_arm64/llvmobjdump-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '0b4628435e6356a55766ea35774a16bd5958902f24f9af6f7a2cb6b396843189',
        'size_bytes': 5644628,
        'generation': 1788861352267416,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/clang-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'e2465b54604c33ac79ec4935d76028a364266a21abe3203a31d16363fbf0de95',
        'size_bytes': 51810300,
        'generation': 1788861377296506,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-tidy-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'dd6359427b46cd357f794d112eea360dc5cf17591e05a8ec2135bf05151ed680',
        'size_bytes': 15138216,
        'generation': 1788861377676861,
        'condition': 'host_os == "win" and checkout_clang_tidy',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '676ae987433b6c4c44dfd839c8bc87f65ae084633688c28a4d1e0902394bcd7e',
        'size_bytes': 2646660,
        'generation': 1788861399534187,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/clangd-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '70a7922b2d0847058d0ec134f434f1c9647059e6549c374482708aa596c4c2a8',
        'size_bytes': 15434148,
        'generation': 1788861377817091,
       'condition': 'host_os == "win" and checkout_clangd',
      },
      {
        'object_name': 'Win/llvm-code-coverage-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': 'ac5264d7410c70271c1b49584b04b96d0c5dbe4548a6ac6f0c430103d9df6307',
        'size_bytes': 2524592,
        'generation': 1788861378309558,
        'condition': 'host_os == "win" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-24-init-7747-g62397f8b-1.tar.xz',
        'sha256sum': '25e3066d503fa5a30925a2354dda8f13c1164ff5d975245b0c5c212be8d0dae9',
        'size_bytes': 6002640,
        'generation': 1788861377946741,
        'condition': '(checkout_linux or checkout_mac or checkout_android) and host_os == "win"',
      },
    ]
  },
  # Update prebuilt Rust toolchain.
  'src/third_party/rust-toolchain': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'condition': 'not rust_force_head_revision',
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-c33d8f3b5a50b56466998e8c5ed8a077d2caed84-1-llvmorg-24-init-7747-g62397f8b.tar.xz',
        'sha256sum': '17758ac240881c51c58a96a81447d847dcd8467f2f97756ec85cfc91a842b872',
        'size_bytes': 276200240,
        'generation': 1788861288435255,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-c33d8f3b5a50b56466998e8c5ed8a077d2caed84-1-llvmorg-24-init-7747-g62397f8b.tar.xz',
        'sha256sum': 'ecec86edd005569c1d69d0645939fc3d799de75e43d3e38187eda8a2266c2416',
        'size_bytes': 263801456,
        'generation': 1788861292135375,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-c33d8f3b5a50b56466998e8c5ed8a077d2caed84-1-llvmorg-24-init-7747-g62397f8b.tar.xz',
        'sha256sum': '3dfebf534ebd1b19bc56e5f910776d3caf1da0a995cd708fe1341fa14ce049d6',
        'size_bytes': 247990220,
        'generation': 1788861296169315,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-c33d8f3b5a50b56466998e8c5ed8a077d2caed84-1-llvmorg-24-init-7747-g62397f8b.tar.xz',
        'sha256sum': '77fd1bf846061da3c29955027dc9a90fb964ca1f0c4d8b712f4b657e197c0849',
        'size_bytes': 416407496,
        'generation': 1788861299836193,
        'condition': 'host_os == "win"',
      },
    ],
  },
  'src/buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux" and non_git_source',
  },
  'src/buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac"',
  },
  'src/buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "win"',
  },

  'src/third_party/compiler-rt/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/compiler-rt.git' + '@' +
    Var('compiler_rt_revision'),
  'src/third_party/libc++/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libcxx.git' + '@' +
    Var('libcxx_revision'),
  'src/third_party/libc++abi/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libcxxabi.git' + '@' +
    Var('libcxxabi_revision'),
  'src/third_party/libunwind/src': {
    'url': Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libunwind.git' + '@' +
        Var('libunwind_revision'),
    'condition': 'checkout_linux',
  },
  'src/third_party/llvm-libc/src':
    Var('chromium_git') +
    '/external/github.com/llvm/llvm-project/libc.git' + '@' +
    Var('llvm_libc_revision'),

  'src/net/third_party/quiche/src':
    Var('quiche_git') + '/quiche.git' + '@' +  Var('quiche_revision'),


  'src/third_party/highway/src':
    Var('chromium_git') + '/external/github.com/google/highway.git' + '@' + Var('highway_revision'),

  'src/third_party/boringssl/src':
    Var('boringssl_git') + '/boringssl.git' + '@' +  Var('boringssl_revision'),

  'src/third_party/catapult':
    Var('chromium_git') + '/catapult.git' + '@' + Var('catapult_revision'),

  'src/third_party/ced/src':
    Var('chromium_git') + '/external/github.com/google/compact_enc_det.git' + '@' + 'd127078cedef9c6642cbe592dacdd2292b50bb19',

  'src/third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git' + '@' + 'ed9c87f6f12f6b87210e7025d4a36a5a72a2ccd4',

  'src/third_party/fontconfig/src': {
      'url': Var('chromium_git') + '/external/fontconfig.git' + '@' + 'd17ee184e436712c2abbe14a9c0ec02fb6acf5c5',
      'condition': 'checkout_linux',
  },

  'src/third_party/freetype/src':
    Var('chromium_git') + '/chromium/src/third_party/freetype2.git' + '@' + Var('freetype_revision'),


  'src/third_party/harfbuzz/src':
    Var('chromium_git') + '/external/github.com/harfbuzz/harfbuzz.git' + '@' + Var('harfbuzz_revision'),

  'src/third_party/emoji-segmenter/src':
    Var('chromium_git') + '/external/github.com/google/emoji-segmenter.git' + '@' + Var('emoji_segmenter_revision'),

  'src/third_party/ots/src':
    Var('chromium_git') + '/external/github.com/khaledhosny/ots.git' + '@' + Var('ots_revision'),

  'src/third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git' + '@' + Var('googletest_revision'),

  # TODO: crbug.com/431264806 - Switch to CIPD package for Windows builds.
  'src/third_party/gperf': {
      'url': Var('chromium_git') + '/chromium/deps/gperf.git' + '@' + 'e9eeea862a18e77b945d98eff7e1bf065d3daf8e',
      'condition': 'checkout_win',
  },

  'src/third_party/gperf/cipd': {
      'packages': [
        {
          'package': 'infra/3pp/tools/gperf/${{platform}}',
          'version': 'version:3@3.2',
        },
      ],
      'condition': 'host_os == "linux" and non_git_source',
      'dep_type': 'cipd',
  },

  # Host platform package. ${platform} folder is not used as in .gn the variable
  # is not initialized yet by the time Python is required.
  'src/third_party/cpython3/host': {
      'packages': [
        {
          'package': 'infra/3pp/tools/cpython3/${{platform}}',
          'version': Var('cpython3_version'),
        },
      ],
      'condition': 'non_git_source',
      'dep_type': 'cipd',
  },

  # Userspace interface to kernel DRM services.
  'src/third_party/libdrm/src': {
      'url': Var('chromium_git') + '/chromiumos/third_party/libdrm.git' + '@' + 'e984d448b8b17aab853369e6c203e53719f46de1',
      'condition': 'checkout_linux',
  },

  'src/third_party/expat/src':
    Var('chromium_git') + '/external/github.com/libexpat/libexpat.git' + '@' + Var('libexpat_revision'),

  'src/third_party/libjpeg_turbo':
    Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git' + '@' + '640f254ad0fa03f6b1f29f89b7dd9366f2f6e533',



  'src/third_party/libwebp/src':
    Var('chromium_git') + '/webm/libwebp.git' + '@' +  'b43b2caa710c0c997c066cb32c7fea1391fad70a',

  'src/third_party/lss': {
      'url': Var('chromium_git') + '/linux-syscall-support.git' + '@' + Var('lss_revision'),
      'condition': 'checkout_android or checkout_linux',
  },

  'src/third_party/material_color_utilities/src': {
      'url': Var('chromium_git') + '/external/github.com/material-foundation/material-color-utilities.git' + '@' + '5b3618b16fdc3825e21d5679bafd144662088ea1',
  },

  'src/third_party/nasm': {
      'url': Var('chromium_git') + '/chromium/deps/nasm.git' + '@' +
      '525a09a813be0f75b646ee93fc2a31c27b87d722'
  },

  'src/third_party/ninja': {
    'packages': [
      {
        'package': Var('ninja_package') + '${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'condition': 'non_git_source',
    'dep_type': 'cipd',
  },



  'src/third_party/re2/src':
    Var('chromium_git') + '/external/github.com/google/re2.git' + '@' + '972a15cedd008d846f1a39b2e88ce48d7f166cbd',




  # Wuffs' canonical repository is at github.com/google/wuffs, but we use
  # Skia's mirror of Wuffs, the same as in upstream Skia's DEPS file.
  #
  # The local directory is called "third_party/wuffs" (matching upstream Skia
  # and other non-Chromium, Skia-using projects) even though the git repo we
  # clone is called "wuffs-mirror-release-c". The reasons for using w-m-r-c are
  # listed in the https://crrev.com/c/3086053 commit message. One reason is
  # that the w-m-r-c subset is much smaller and changes much less frequently.
  'src/third_party/wuffs/src':
    Var('skia_git') + '/external/github.com/google/wuffs-mirror-release-c.git' + '@' +  Var('wuffs_revision'),

}


include_rules = [
  # Everybody can use some things.
  # NOTE: THIS HAS TO STAY IN SYNC WITH third_party/DEPS which disallows these.
  '+base',
  '+build',
  '+ipc',

  # PartitionAlloc is located at `base/allocator/partition_allocator` but
  # prefers its own include path:
  # `#include "partition_alloc/..."` is prefered to
  # `#include "base/allocator/partition_allocator/src/partition_alloc/..."`.
  "+partition_alloc",
  "-base/allocator/partition_allocator",

  '+testing',
  '+third_party/jni_zero',
  '+third_party/icu/source/common/unicode',
  '+third_party/icu/source/i18n/unicode',
  '+url',

  # Everybody can use support libraries for C++/Rust interop based on Crubit.
  #
  # Actual headers location (under `third_party/rust-toolchain/lib`) should be
  # treated as an implementation detail - code should instead use
  # `third_party/crubit/support/...` as the canonical `#include` path (this is
  # unified across other major Crubit clients).
  '+third_party/crubit/support',

  # Abseil is allowed by default, but some features are banned. See
  # //styleguide/c++/c++-features.md.
  '+third_party/abseil-cpp',
  '-third_party/abseil-cpp/absl/algorithm/container.h',
  '-third_party/abseil-cpp/absl/base/attributes.h',
  '-third_party/abseil-cpp/absl/base/no_destructor.h',
  '-third_party/abseil-cpp/absl/base/nullability.h',
  '-third_party/abseil-cpp/absl/base/throw_delegate.h',
  '-third_party/abseil-cpp/absl/container/btree_map.h',
  '-third_party/abseil-cpp/absl/container/btree_set.h',
  '-third_party/abseil-cpp/absl/container/linked_hash_map.h',
  '-third_party/abseil-cpp/absl/container/linked_hash_set.h',
  '-third_party/abseil-cpp/absl/flags',
  '-third_party/abseil-cpp/absl/functional/any_invocable.h',
  '-third_party/abseil-cpp/absl/functional/bind_back.h',
  '-third_party/abseil-cpp/absl/functional/bind_front.h',
  '-third_party/abseil-cpp/absl/functional/function_ref.h',
  '-third_party/abseil-cpp/absl/hash',
  '+third_party/abseil-cpp/absl/hash/hash_testing.h',
  '-third_party/abseil-cpp/absl/log',
  '-third_party/abseil-cpp/absl/random',
  '-third_party/abseil-cpp/absl/status/statusor.h',
  '-third_party/abseil-cpp/absl/strings',
  '+third_party/abseil-cpp/absl/strings/ascii.h',
  '+third_party/abseil-cpp/absl/strings/cord.h',
  '+third_party/abseil-cpp/absl/strings/str_format.h',
  '-third_party/abseil-cpp/absl/synchronization',
  '-third_party/abseil-cpp/absl/time',
  '-third_party/abseil-cpp/absl/types/any.h',
  '-third_party/abseil-cpp/absl/types/any_span.h',
  '-third_party/abseil-cpp/absl/types/optional.h',
  '-third_party/abseil-cpp/absl/types/optional_ref.h',
  '-third_party/abseil-cpp/absl/types/source_location.h',
  '-third_party/abseil-cpp/absl/types/span.h',
  '-third_party/abseil-cpp/absl/types/variant.h',
  '-third_party/abseil-cpp/absl/utility/utility.h',
]


# checkdeps.py shouldn't check include paths for files in these dirs:
skip_child_includes = [
  'out',
  'skia',
  'testing',
  'third_party/abseil-cpp',
  'v8',
]


hooks = [
  # GN uses the DEPS-provided CPython, not the repository-wide vpython spec.
  # Do not eagerly install Chromium's analysis/test/cloud Python environment
  # on every screenshot CI runner. Explicit vpython users resolve it on demand.
  {
    # This clobbers when necessary (based on get_landmines.py). This should
    # run as early as possible so that other things that get/generate into the
    # output directory will not subsequently be clobbered.
    'name': 'landmines',
    'pattern': '.',
    'action': [
        'python3',
        'src/build/landmines.py',
    ],
  },
  {
    # Ensure that the DEPS'd "depot_tools" has its self-update capability
    # disabled.
    'name': 'disable_depot_tools_selfupdate',
    'pattern': '.',
    'action': [
        'python3',
        'src/third_party/depot_tools/update_depot_tools_toggle.py',
        '--disable',
    ],
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'src/build/vs_toolchain.py', 'update', '--force'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac or checkout_ios',
    'action': ['python3', 'src/build/mac_toolchain.py'],
  },
  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    # --filter picks the commit LASTCHANGE describes. Upstream's default
    # '^Change-Id:' finds the last upstream commit; this fork's commits carry
    # none, and '.' (any commit) made LASTCHANGE follow HEAD. That hash is read
    # by nothing this engine ships, but it is an input of base's
    # check_version_internal.h, and a new value every CI run recompiled
    # libbase, relinked every host tool (protoc, nasm, the perfetto plugins),
    # regenerated their outputs and recompiled ~500 Blink units on a "warm"
    # cache. Pinning the filter to the root commit's subject makes LASTCHANGE
    # and LASTCHANGE.committime (the build timestamp) constants.
    'action': ['python3', 'src/build/util/lastchange.py',
               '--filter', '^shotium: static screenshots from a stripped Chromium$',
               '-o', 'src/build/util/LASTCHANGE'],
  },
  {
    # Update lastchange_commit_position.h (only for CrOS).
    'name': 'lastchange_commit_position_cros',
    'pattern': '.',
    'condition': 'checkout_chromeos',
    'action': ['python3', 'src/build/util/lastchange.py',
               '-m', 'CHROMIUM',
               '--commit-position-header',
               'src/build/util/LASTCHANGE_commit_position.h'],
  },
  # Pull dsymutil binaries using checked-in hashes.
  {
    'name': 'dsymutil_mac_arm64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "arm64"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang',
                '-s', 'src/tools/clang/dsymutil/bin/dsymutil.arm64.sha1',
                '-o', 'src/tools/clang/dsymutil/bin/dsymutil',
    ],
  },
  {
    'name': 'dsymutil_mac_x64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "x64"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang',
                '-s', 'src/tools/clang/dsymutil/bin/dsymutil.x64.sha1',
                '-o', 'src/tools/clang/dsymutil/bin/dsymutil',
    ],
  },

  # Pull rc binaries using checked-in hashes.
  {
    'name': 'rc_win',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "win"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'src/build/toolchain/win/rc/win/rc.exe.sha1',
    ],
  },
  {
    'name': 'rc_mac',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "mac"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'src/build/toolchain/win/rc/mac/rc.sha1',
    ],
  },
  {
    'name': 'rc_linux',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "linux"',
    'action': [ 'python3',
                'src/third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'src/build/toolchain/win/rc/linux64/rc.sha1',
    ]
  },

]

# Add any corresponding DEPS files from this list to chromium.exclusions in
# //testing/buildbot/trybot_analyze_config.json
# ctx: https://crbug.com/1201994
recursedeps = []
