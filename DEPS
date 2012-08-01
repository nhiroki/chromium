# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

vars = {
  # Use this googlecode_url variable only if there is an internal mirror for it.
  # If you do not know, use the full path while defining your new deps entry.
  "googlecode_url": "http://%s.googlecode.com/svn",
  "sourceforge_url": "http://%(repo)s.svn.sourceforge.net/svnroot/%(repo)s",
  "webkit_trunk": "http://svn.webkit.org/repository/webkit/trunk",
  "nacl_trunk": "http://src.chromium.org/native_client/trunk",
  "webkit_revision": "124188",
  "chromium_git": "http://git.chromium.org/git",
  "chromiumos_git": "http://git.chromium.org/chromiumos",
  "swig_revision": "69281",
  "nacl_revision": "9317",
  # After changing nacl_revision, run 'glient sync' and check native_client/DEPS
  # to update other nacl_*_revision's.
  "nacl_tools_revision": "9286",  # native_client/DEPS: tools_rev
  "gtm_revision": "534",

  # These hashes need to be updated when nacl_toolchain_revision is changed.
  # After changing nacl_toolchain_revision, run 'gclient runhooks' to get the
  # new values.
  "nacl_toolchain_linux_x86_hash":
      "5a013540b816336f6f2af26c525f8d9f8e20e144",
  "nacl_toolchain_linux_x86_newlib_hash":
      "c3598baef5a49e6ce8ca768d7f01383641d56025",
  "nacl_toolchain_mac_x86_hash":
      "664e057b1507923b0526ebb7294deb8c31cb8bfc",
  "nacl_toolchain_mac_x86_newlib_hash":
      "93f3a7ca6f97b093380ab4c305b2be42e89004c0",
  "nacl_toolchain_pnacl_linux_x86_32_hash":
      "025f9b2c1fcb9eb142cb30225526fcc443091e19",
  "nacl_toolchain_pnacl_linux_x86_64_hash":
      "fd5f7190d2462155a5a492980df99327a7733df9",
  "nacl_toolchain_pnacl_mac_x86_32_hash":
      "6fabbbec2ec6c6854d21c95cda82284d8c3eabcc",
  "nacl_toolchain_pnacl_translator_hash":
      "17fef3ab59d1445ec02ae571bf6f96adfc4d609b",
  "nacl_toolchain_pnacl_win_x86_32_hash":
      "ef27427f000f66249a75eef028ef30d57056e9a7",
  "nacl_toolchain_win_x86_hash":
      "9a854462e601338153bc65bfdc1171a87dc0bfe6",
  "nacl_toolchain_win_x86_newlib_hash":
      "c4dd3f8b65e309d242a3bfce89a15f580ef2e4f0",
  "nacl_toolchain_revision": "9170",
  "pnacl_toolchain_revision": "9299",

  "libjingle_revision": "163",
  "libphonenumber_revision": "456",
  "libvpx_revision": "149032",
  "lss_revision": "11",

  # These two FFmpeg variables must be updated together.  One is used for SVN
  # checkouts and the other for Git checkouts.
  "ffmpeg_revision": "148611",
  "ffmpeg_hash": "4287c0271f399b98eb6851e6e650098f26b8c3c2",

  "sfntly_revision": "134",
  "skia_revision": "4834",
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Skia
  # and V8 without interference from each other.
  "v8_revision": "12227",
  "webrtc_revision": "2538",
  "jsoncpp_revision": "248",
  "nss_revision": "145873",
}

deps = {
  "src/breakpad/src":
    (Var("googlecode_url") % "google-breakpad") + "/trunk/src@999",

  "src/googleurl":
    (Var("googlecode_url") % "google-url") + "/trunk@175",

  "src/sandbox/linux/seccomp-legacy":
    (Var("googlecode_url") % "seccompsandbox") + "/trunk@186",

  "src/sdch/open-vcdiff":
    (Var("googlecode_url") % "open-vcdiff") + "/trunk@42",

  "src/testing/gtest":
    (Var("googlecode_url") % "googletest") + "/trunk@617",

  "src/testing/gmock":
    (Var("googlecode_url") % "googlemock") + "/trunk@405",

  "src/third_party/angle":
    (Var("googlecode_url") % "angleproject") + "/trunk@1243",

  "src/third_party/trace-viewer":
    (Var("googlecode_url") % "trace-viewer") + "/trunk@103",

  # Note that this is *not* where we check out WebKit -- this just
  # puts some extra files into place for the real WebKit checkout to
  # happen.  See lines mentioning "webkit_revision" for the real
  # WebKit checkout.
  "src/third_party/WebKit":
    "/trunk/deps/third_party/WebKit@76115",

  "src/third_party/icu":
    "/trunk/deps/third_party/icu46@146527",

  "src/third_party/libexif/sources":
    "/trunk/deps/third_party/libexif/sources@146817",

  "src/third_party/hunspell":
   "/trunk/deps/third_party/hunspell@132738",

  "src/third_party/hunspell_dictionaries":
    "/trunk/deps/third_party/hunspell_dictionaries@138928",

  "src/third_party/safe_browsing/testing":
    (Var("googlecode_url") % "google-safe-browsing") + "/trunk/testing@110",

  "src/third_party/cacheinvalidation/files/src/google":
    (Var("googlecode_url") % "google-cache-invalidation-api") +
    "/trunk/src/google@219",

  "src/third_party/leveldatabase/src":
    (Var("googlecode_url") % "leveldb") + "/trunk@67",

  "src/third_party/snappy/src":
    (Var("googlecode_url") % "snappy") + "/trunk@63",

  "src/tools/grit":
    (Var("googlecode_url") % "grit-i18n") + "/trunk@63",

  "src/tools/gyp":
    (Var("googlecode_url") % "gyp") + "/trunk@1441",

  "src/v8":
    (Var("googlecode_url") % "v8") + "/trunk@" + Var("v8_revision"),

  "src/native_client":
    Var("nacl_trunk") + "/src/native_client@" + Var("nacl_revision"),

  "src/native_client_sdk/src/site_scons":
    Var("nacl_trunk") + "/src/native_client/site_scons@" + Var("nacl_revision"),

  "src/third_party/pymox/src":
    (Var("googlecode_url") % "pymox") + "/trunk@70",

  "src/chrome/test/data/extensions/api_test/permissions/nacl_enabled/bin":
    Var("nacl_trunk") + "/src/native_client/tests/prebuilt@" +
    Var("nacl_revision"),

  "src/third_party/sfntly/cpp/src":
    (Var("googlecode_url") % "sfntly") + "/trunk/cpp/src@" +
    Var("sfntly_revision"),

  "src/third_party/skia/src":
    (Var("googlecode_url") % "skia") + "/trunk/src@" + Var("skia_revision"),

  "src/third_party/skia/include":
    (Var("googlecode_url") % "skia") + "/trunk/include@" + Var("skia_revision"),

  "src/third_party/WebKit/LayoutTests":
    Var("webkit_trunk") + "/LayoutTests@" + Var("webkit_revision"),

  "src/third_party/WebKit/Source":
    Var("webkit_trunk") + "/Source@" + Var("webkit_revision"),

  "src/third_party/WebKit/Tools/DumpRenderTree":
    Var("webkit_trunk") + "/Tools/DumpRenderTree@" + Var("webkit_revision"),

  "src/third_party/WebKit/Tools/Scripts":
    Var("webkit_trunk") + "/Tools/Scripts@" + Var("webkit_revision"),

  "src/third_party/WebKit/Tools/TestWebKitAPI":
    Var("webkit_trunk") + "/Tools/TestWebKitAPI@" + Var("webkit_revision"),

  "src/third_party/ots":
    (Var("googlecode_url") % "ots") + "/trunk@94",

  "src/tools/page_cycler/acid3":
    "/trunk/deps/page_cycler/acid3@102714",

  "src/chrome/test/data/perf/canvas_bench":
    "/trunk/deps/canvas_bench@122605",

  "src/chrome/test/data/perf/frame_rate/content":
    "/trunk/deps/frame_rate/content@93671",

  "src/third_party/bidichecker":
    (Var("googlecode_url") % "bidichecker") + "/trunk/lib@4",

  "src/third_party/v8-i18n":
    (Var("googlecode_url") % "v8-i18n") + "/trunk@105",

  # When roll to another webgl conformance tests revision, please goto
  # chrome/test/gpu and run generate_webgl_conformance_test_list.py.
  "src/third_party/webgl_conformance":
    "/trunk/deps/third_party/webgl/sdk/tests@148561",

  # We run these layout tests as UI tests. Since many of the buildbots that
  # run layout tests do NOT have access to the LayoutTest directory, we need
  # to map them here. In practice, these do not take up much space.
  "src/content/test/data/layout_tests/LayoutTests/fast/events":
    Var("webkit_trunk") + "/LayoutTests/fast/events@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/fast/js/resources":
    Var("webkit_trunk") + "/LayoutTests/fast/js/resources@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/fast/workers":
    Var("webkit_trunk") + "/LayoutTests/fast/workers@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/http/tests/resources":
    Var("webkit_trunk") + "/LayoutTests/http/tests/resources@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/http/tests/workers":
    Var("webkit_trunk") + "/LayoutTests/http/tests/workers@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/http/tests/xmlhttprequest":
    Var("webkit_trunk") + "/LayoutTests/http/tests/xmlhttprequest@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/http/tests/websocket/tests":
    Var("webkit_trunk") + "/LayoutTests/http/tests/websocket/tests@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium/fast/workers":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium/fast/workers@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium/fast/events":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium/fast/events@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/fast/events":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium-win/fast/events@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/fast/workers":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium-win/fast/workers@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/http/tests/appcache":
    Var("webkit_trunk") + "/LayoutTests/http/tests/appcache@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/http/tests/workers":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium-win/http/tests/workers@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/storage/domstorage":
    Var("webkit_trunk") + "/LayoutTests/platform/chromium-win/storage/domstorage@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/storage/domstorage":
    Var("webkit_trunk") + "/LayoutTests/storage/domstorage@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/storage/indexeddb":
    Var("webkit_trunk") + "/LayoutTests/storage/indexeddb@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/fast/filesystem/resources":
    Var("webkit_trunk") + "/LayoutTests/fast/filesystem/resources@" +
    Var("webkit_revision"),
  "src/content/test/data/layout_tests/LayoutTests/media":
    Var("webkit_trunk") + "/LayoutTests/media@" +
    Var("webkit_revision"),

  "src/third_party/swig/Lib":
    "/trunk/deps/third_party/swig/Lib@" + Var("swig_revision"),

  # Make sure you update the two functional.DEPS and webdriver.DEPS too.
  "src/third_party/webdriver/pylib":
    (Var("googlecode_url") % "selenium") + "/trunk/py@16922",

  "src/third_party/libvpx":
    "/trunk/deps/third_party/libvpx@" +
    Var("libvpx_revision"),

  "src/third_party/ffmpeg":
    "/trunk/deps/third_party/ffmpeg@" +
    Var("ffmpeg_revision"),

  "src/third_party/libjingle/source":
    (Var("googlecode_url") % "libjingle") + "/trunk@" +
    Var("libjingle_revision"),

  "src/third_party/libsrtp":
    "/trunk/deps/third_party/libsrtp@123853",

  "src/third_party/speex":
    "/trunk/deps/third_party/speex@111570",

  "src/third_party/yasm/source/patched-yasm":
    "/trunk/deps/third_party/yasm/patched-yasm@134927",

  "src/third_party/libjpeg_turbo":
    "/trunk/deps/third_party/libjpeg_turbo@147428",

  "src/third_party/flac":
    "/trunk/deps/third_party/flac@120197",

  "src/third_party/pyftpdlib/src":
    (Var("googlecode_url") % "pyftpdlib") + "/trunk@977",

  "src/third_party/scons-2.0.1":
    Var("nacl_trunk") + "/src/third_party/scons-2.0.1@" +
        Var("nacl_tools_revision"),

  "src/third_party/webrtc":
    (Var("googlecode_url") % "webrtc") + "/stable/src@" + Var("webrtc_revision"),

  "src/third_party/jsoncpp/source/include":
    (Var("sourceforge_url") % {"repo": "jsoncpp"}) +
        "/trunk/jsoncpp/include@" + Var("jsoncpp_revision"),

  "src/third_party/jsoncpp/source/src/lib_json":
    (Var("sourceforge_url") % {"repo": "jsoncpp"}) +
        "/trunk/jsoncpp/src/lib_json@" + Var("jsoncpp_revision"),

  "src/third_party/libyuv":
    (Var("googlecode_url") % "libyuv") + "/trunk@311",

  "src/third_party/mozc/session":
    (Var("googlecode_url") % "mozc") + "/trunk/src/session@83",

  "src/third_party/mozc/chrome/chromeos/renderer":
    (Var("googlecode_url") % "mozc") + "/trunk/src/chrome/chromeos/renderer@83",

  "src/third_party/smhasher/src":
    (Var("googlecode_url") % "smhasher") + "/trunk@146",

  "src/third_party/libphonenumber/src/phonenumbers":
     (Var("googlecode_url") % "libphonenumber") +
         "/trunk/cpp/src/phonenumbers@" + Var("libphonenumber_revision"),
  "src/third_party/libphonenumber/src/test":
     (Var("googlecode_url") % "libphonenumber") + "/trunk/cpp/test@" +
         Var("libphonenumber_revision"),
  "src/third_party/libphonenumber/src/resources":
     (Var("googlecode_url") % "libphonenumber") + "/trunk/resources@" +
         Var("libphonenumber_revision"),

  "src/third_party/undoview":
    "/trunk/deps/third_party/undoview@119694",

  "src/tools/deps2git":
    "/trunk/tools/deps2git@148781",

  "src/third_party/webpagereplay":
    (Var("googlecode_url") % "web-page-replay") + "/trunk@489",
}


deps_os = {
  "win": {
    "src/chrome/tools/test/reference_build/chrome_win":
      "/trunk/deps/reference_builds/chrome_win@137747",

    "src/third_party/cygwin":
      "/trunk/deps/third_party/cygwin@133786",

    "src/third_party/python_26":
      "/trunk/tools/third_party/python_26@89111",

    "src/third_party/psyco_win32":
      "/trunk/deps/third_party/psyco_win32@79861",

    "src/third_party/bison":
      "/trunk/deps/third_party/bison@147303",

    "src/third_party/gperf":
      "/trunk/deps/third_party/gperf@147304",

    "src/third_party/perl":
      "/trunk/deps/third_party/perl@147900",

    "src/third_party/lighttpd":
      "/trunk/deps/third_party/lighttpd@33727",

    # Chrome Frame related deps
    "src/third_party/xulrunner-sdk":
      "/trunk/deps/third_party/xulrunner-sdk@119756",
    "src/chrome_frame/tools/test/reference_build/chrome_win":
      "/trunk/deps/reference_builds/chrome_win@89574",

    # Parses Windows PE/COFF executable format.
    "src/third_party/pefile":
      (Var("googlecode_url") % "pefile") + "/trunk@63",

    # NSS, for SSLClientSocketNSS.
    "src/third_party/nss":
      "/trunk/deps/third_party/nss@" + Var("nss_revision"),

    "src/third_party/swig/win":
      "/trunk/deps/third_party/swig/win@" + Var("swig_revision"),

    # GNU binutils assembler for x86-32.
    "src/third_party/gnu_binutils":
      (Var("nacl_trunk") + "/deps/third_party/gnu_binutils@" +
       Var("nacl_tools_revision")),
    # GNU binutils assembler for x86-64.
    "src/third_party/mingw-w64/mingw/bin":
      (Var("nacl_trunk") + "/deps/third_party/mingw-w64/mingw/bin@" +
       Var("nacl_tools_revision")),

    # Dependencies used by libjpeg-turbo
    "src/third_party/yasm/binaries":
      "/trunk/deps/third_party/yasm/binaries@74228",

    # Binary level profile guided optimizations. This points to the
    # latest release binaries for the toolchain.
    "src/third_party/syzygy/binaries":
      (Var("googlecode_url") % "sawbuck") + "/trunk/syzygy/binaries@991",

    # Binaries for nacl sdk.
    "src/third_party/nacl_sdk_binaries":
      "/trunk/deps/third_party/nacl_sdk_binaries@111576",
  },
  "ios": {
    "src/third_party/GTM":
      (Var("googlecode_url") % "google-toolbox-for-mac") + "/trunk@" +
      Var("gtm_revision"),

    "src/third_party/nss":
      "/trunk/deps/third_party/nss@" + Var("nss_revision"),

    # class-dump utility to generate header files for undocumented SDKs
    "src/testing/iossim/third_party/class-dump":
      "/trunk/deps/third_party/class-dump@147231",

    # Code that's not needed due to not building everything (especially WebKit).
    "src/build/util/support": None,
    "src/chrome/test/data/extensions/api_test/permissions/nacl_enabled/bin": None,
    "src/content/test/data/layout_tests/LayoutTests/fast/events": None,
    "src/content/test/data/layout_tests/LayoutTests/fast/filesystem/resources": None,
    "src/content/test/data/layout_tests/LayoutTests/fast/js/resources": None,
    "src/content/test/data/layout_tests/LayoutTests/fast/workers": None,
    "src/content/test/data/layout_tests/LayoutTests/http/tests/appcache": None,
    "src/content/test/data/layout_tests/LayoutTests/http/tests/resources": None,
    "src/content/test/data/layout_tests/LayoutTests/http/tests/websocket/tests": None,
    "src/content/test/data/layout_tests/LayoutTests/http/tests/workers": None,
    "src/content/test/data/layout_tests/LayoutTests/http/tests/xmlhttprequest": None,
    "src/content/test/data/layout_tests/LayoutTests/media": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/fast/events": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/fast/workers": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/http/tests/workers": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium-win/storage/domstorage": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium/fast/events": None,
    "src/content/test/data/layout_tests/LayoutTests/platform/chromium/fast/workers": None,
    "src/content/test/data/layout_tests/LayoutTests/storage/indexeddb": None,
    "src/content/test/data/layout_tests/LayoutTests/storage/domstorage": None,
    "src/chrome/test/data/perf/canvas_bench": None,
    "src/chrome/test/data/perf/frame_rate/content": None,
    "src/native_client": None,
    "src/native_client/src/third_party/ppapi": None,
    "src/native_client_sdk/src/site_scons": None,
    "src/sandbox/linux/seccomp-legacy": None,
    "src/third_party/angle": None,
    "src/third_party/bidichecker": None,
    "src/third_party/webgl_conformance": None,
    "src/third_party/ffmpeg": None,
    "src/third_party/hunspell_dictionaries": None,
    "src/third_party/hunspell": None,
    "src/third_party/leveldatabase/src": None,
    "src/third_party/libexif/sources": None,
    "src/third_party/libjingle/source": None,
    "src/third_party/libjpeg_turbo": None,
    "src/third_party/libphonenumber/src/phonenumbers": None,
    "src/third_party/libphonenumber/src/test": None,
    "src/third_party/libphonenumber/src/resources": None,
    "src/third_party/jsoncpp/source/include": None,
    "src/third_party/jsoncpp/source/src/lib_json": None,
    "src/third_party/libsrtp": None,
    "src/third_party/libvpx": None,
    "src/third_party/libyuv": None,
    "src/third_party/mozc/session": None,
    "src/third_party/mozc/chrome/chromeos/renderer": None,
    "src/third_party/ots": None,
    "src/third_party/pylib": None,
    "src/third_party/pymox/src": None,
    "src/third_party/safe_browsing/testing": None,
    "src/third_party/scons-2.0.1": None,
    "src/third_party/sfntly/cpp/src": None,
    "src/third_party/skia/src": None,
    "src/third_party/smhasher/src": None,
    "src/third_party/snappy/src": None,
    "src/third_party/swig/Lib": None,
    "src/third_party/undoview": None,
    "src/third_party/v8-i18n": None,
    "src/third_party/webdriver/pylib": None,
    "src/third_party/webpagereplay": None,
    "src/third_party/webrtc": None,
    "src/third_party/WebKit": None,
    "src/third_party/WebKit/LayoutTests": None,
    "src/third_party/WebKit/Source": None,
    "src/third_party/WebKit/Tools/DumpRenderTree": None,
    "src/third_party/WebKit/Tools/Scripts": None,
    "src/third_party/WebKit/Tools/TestWebKitAPI": None,
    "src/third_party/yasm/source/patched-yasm": None,
    "src/tools/page_cycler/acid3": None,
    "src/v8": None,
  },
  "mac": {
    "src/chrome/tools/test/reference_build/chrome_mac":
      "/trunk/deps/reference_builds/chrome_mac@137727",

    "src/third_party/GTM":
      (Var("googlecode_url") % "google-toolbox-for-mac") + "/trunk@" +
      Var("gtm_revision"),
    "src/third_party/pdfsqueeze":
      (Var("googlecode_url") % "pdfsqueeze") + "/trunk@5",
    "src/third_party/lighttpd":
      "/trunk/deps/third_party/lighttpd@33737",

    "src/third_party/swig/mac":
      "/trunk/deps/third_party/swig/mac@" + Var("swig_revision"),

    # NSS, for SSLClientSocketNSS.
    "src/third_party/nss":
      "/trunk/deps/third_party/nss@" + Var("nss_revision"),

    "src/chrome/installer/mac/third_party/xz/xz":
      "/trunk/deps/third_party/xz@87706",
  },
  "unix": {
    # Linux, really.
    "src/chrome/tools/test/reference_build/chrome_linux":
      "/trunk/deps/reference_builds/chrome_linux@137712",

    "src/third_party/xdg-utils":
      "/trunk/deps/third_party/xdg-utils@93299",

    "src/third_party/swig/linux":
      "/trunk/deps/third_party/swig/linux@" + Var("swig_revision"),

    "src/third_party/lss":
      ((Var("googlecode_url") % "linux-syscall-support") + "/trunk/lss@" +
       Var("lss_revision")),

    "src/third_party/openssl":
      "/trunk/deps/third_party/openssl@130472",

    "src/third_party/WebKit/Tools/gdb":
      Var("webkit_trunk") + "/Tools/gdb@" + Var("webkit_revision"),

    "src/third_party/gold":
      "/trunk/deps/third_party/gold@124239",

    # For Chromium OS.
    "src/third_party/cros_system_api":
      Var("chromiumos_git") + "/platform/system_api.git" +
      "@715c98d7d0ca9a1f0576fca77cb4a49498d303ae",
  },
  "android": {
    "src/third_party/freetype":
      Var("chromium_git") + "/chromium/src/third_party/freetype.git" +
      "@1f74e4e7ad3ca4163b4578fc30da26a165dd55e7",

    "src/third_party/aosp":
      "/trunk/deps/third_party/aosp@148330",

    "src/third_party/android_tools":
      Var("chromium_git") + "/android_tools.git" +
      "@21f9993b4ce955f1af6a88e5dc1135ba645920a7",
  },
}


include_rules = [
  # Everybody can use some things.
  "+base",
  "+build",
  "+googleurl",
  "+ipc",

  # For now, we allow ICU to be included by specifying "unicode/...", although
  # this should probably change.
  "+unicode",
  "+testing",
]


# checkdeps.py shouldn't check include paths for files in these dirs:
skip_child_includes = [
  "breakpad",
  "chrome_frame",
  "delegate_execute",
  "metro_driver",
  "native_client_sdk",
  "o3d",
  "pdf",
  "sdch",
  "skia",
  "testing",
  "third_party",
  "v8",
]


hooks = [
  {
    # This downloads binaries for Native Client's newlib toolchain.
    # Done in lieu of building the toolchain from scratch as it can take
    # anywhere from 30 minutes to 4 hours depending on platform to build.
    "pattern": ".",
    "action": [
        "python", "src/build/download_nacl_toolchains.py",
         "--no-arm-trusted",
         "--optional-pnacl",
         "--pnacl-version", Var("pnacl_toolchain_revision"),
         "--file-hash", "pnacl_linux_x86_32",
             Var("nacl_toolchain_pnacl_linux_x86_32_hash"),
         "--file-hash", "pnacl_linux_x86_64",
             Var("nacl_toolchain_pnacl_linux_x86_64_hash"),
         "--file-hash", "pnacl_translator",
             Var("nacl_toolchain_pnacl_translator_hash"),
         "--file-hash", "pnacl_mac_x86_32",
             Var("nacl_toolchain_pnacl_mac_x86_32_hash"),
         "--file-hash", "pnacl_win_x86_32",
             Var("nacl_toolchain_pnacl_win_x86_32_hash"),
         "--x86-version", Var("nacl_toolchain_revision"),
         "--file-hash", "mac_x86_newlib",
             Var("nacl_toolchain_mac_x86_newlib_hash"),
         "--file-hash", "win_x86_newlib",
             Var("nacl_toolchain_win_x86_newlib_hash"),
         "--file-hash", "linux_x86_newlib",
             Var("nacl_toolchain_linux_x86_newlib_hash"),
         "--file-hash", "mac_x86",
             Var("nacl_toolchain_mac_x86_hash"),
         "--file-hash", "win_x86",
             Var("nacl_toolchain_win_x86_hash"),
         "--file-hash", "linux_x86",
             Var("nacl_toolchain_linux_x86_hash"),
         "--save-downloads-dir",
             "src/native_client_sdk/src/build_tools/toolchain_archives",
         "--keep",
    ],
  },
  {
    # Pull clang on mac. If nothing changed, or on non-mac platforms, this takes
    # zero seconds to run. If something changed, it downloads a prebuilt clang,
    # which takes ~20s, but clang speeds up builds by more than 20s.
    "pattern": ".",
    "action": ["python", "src/tools/clang/scripts/update.py", "--mac-only"],
  },
  {
    # Update the cygwin mount on Windows.
    "pattern": ".",
    "action": ["python", "src/build/win/setup_cygwin_mount.py", "--win-only"],
  },
  {
    # Update LASTCHANGE. This is also run by export_tarball.py in
    # src/tools/export_tarball - please keep them in sync.
    "pattern": ".",
    "action": ["python", "src/build/util/lastchange.py",
               "-o", "src/build/util/LASTCHANGE"],
  },
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "src/build/gyp_chromium"],
  },
]
