load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

load("@//bazel:omnetpp_deps_step_1.bzl", "omnetpp_deps_step_1")
omnetpp_deps_step_1()

load("@//bazel:omnetpp_deps_step_2.bzl", "omnetpp_deps_step_2")
omnetpp_deps_step_2()

load("@//bazel:omnetpp_deps_step_3.bzl", "omnetpp_deps_step_3")
omnetpp_deps_step_3()

# Isekai external deps
all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

# ABSL
http_archive(
    name = "com_google_absl",
    url = "https://github.com/abseil/abseil-cpp/archive/refs/tags/20230125.3.tar.gz",
    sha256 = "5366d7e7fa7ba0d915014d387b66d0d002c03236448e1ba9ef98122c13b35c36",
    strip_prefix = "abseil-cpp-20230125.3",
)

# glog
http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

http_archive(
    name = "com_google_glog",
    sha256 = "21bc744fb7f2fa701ee8db339ded7dce4f975d0d55837a97be7d46e8382dea5a",
    strip_prefix = "glog-0.5.0",
    urls = ["https://github.com/google/glog/archive/v0.5.0.zip"],
)

#crc32
http_archive(
    name = "com_google_crc32c",
    sha256 = "ac07840513072b7fcebda6e821068aa04889018f24e10e46181068fb214d7e56",
    build_file_content = all_content,
    patches = ["//bazel:crc32c.patch"],
    urls = ["https://github.com/google/crc32c/archive/refs/tags/1.1.2.tar.gz"],
    strip_prefix = "crc32c-1.1.2",
)

#folly
http_archive(
    name = "com_github_nelhage_rules_boost",
    sha256 = "0a1d884aa13201b705f93b86d2d0be1de867f6e592de3c4a3bbe6d04bdddf593",
    strip_prefix = "rules_boost-a32cad61d9166d28ed86d0e07c0d9bca8db9cb82",
    urls = ["https://github.com/nelhage/rules_boost/archive/a32cad61d9166d28ed86d0e07c0d9bca8db9cb82.tar.gz"],
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

http_archive(
    name = "folly",
    sha256 = "a8d95df3fbdeb1ef4d078ee2b2883f5451a62d30526ab5571618f5d13d54406f",
    urls = ["https://github.com/facebook/folly/archive/refs/tags/v2021.11.29.00.tar.gz"],
    strip_prefix = "folly-2021.11.29.00",
    build_file = "@//bazel:folly.BUILD",
)

#protobuf
http_archive(
    name = "com_google_protobuf",
    sha256 = "4a045294ec76cb6eae990a21adb5d8b3c78be486f1507faa601541d1ccefbd6b",
    strip_prefix = "protobuf-3.19.0",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.19.0.tar.gz",
    ],
)

#re2
http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "7a9a4824958586980926a300b4717202485c4b4115ac031822e29aa4ef207e48",
    strip_prefix = "re2-2023-03-01",
    urls = [
        "https://github.com/google/re2/archive/refs/tags/2023-03-01.tar.gz",
    ],
)

#tensorflow
http_archive(
    name = "org_tensorflow",
    sha256 = "75b2e40a9623df32da16d8e97528f5e02e4a958e23b1f2ee9637be8eec5d021b",
    strip_prefix = "tensorflow-2.7.4",
    urls = [
        "https://github.com/tensorflow/tensorflow/archive/refs/tags/v2.7.4.tar.gz",
    ],
)

http_archive(
    name = "highwayhash",
    build_file = "@com_google_riegeli//third_party:highwayhash.BUILD",
    sha256 = "cf891e024699c82aabce528a024adbe16e529f2b4e57f954455e0bf53efae585",
    strip_prefix = "highwayhash-276dd7b4b6d330e4734b756e97ccfb1b69cc2e12",
    urls = ["https://github.com/google/highwayhash/archive/276dd7b4b6d330e4734b756e97ccfb1b69cc2e12.zip"],
)

#Initialize TensorFlow's external dependencies.
load("@org_tensorflow//tensorflow:workspace3.bzl", "workspace")
workspace()

load("@org_tensorflow//tensorflow:workspace2.bzl", "workspace")
workspace()

load("@org_tensorflow//tensorflow:workspace1.bzl", "workspace")
workspace()

load("@org_tensorflow//tensorflow:workspace0.bzl", "workspace")
workspace()

#googletest
http_archive(
    name = "com_google_googletest",
    sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
    strip_prefix = "googletest-release-1.11.0",
    urls = [
        "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
    ],
)

#riegeli
http_archive(
    name = "com_google_riegeli",
    sha256 = "32f303a9b0b6e07101a7a95a4cc364fb4242f0f7431de5da1a2e0ee61f5924c5",
    strip_prefix = "riegeli-562f26cbb685aae10b7d32e32fb53d2e42a5d8c2",
    url = "https://github.com/google/riegeli/archive/562f26cbb685aae10b7d32e32fb53d2e42a5d8c2.zip",
)

#external deps for riegeli
http_archive(
    name = "org_brotli",
    sha256 = "dbeef368f7c5b779ed34f0a6e17e0a55084fb91c49053f91ea23d0680eb39e00",
    strip_prefix = "brotli-27dd7265403d8e8fed99a854b9c3e1db7d79525f",
    urls = ["https://github.com/google/brotli/archive/27dd7265403d8e8fed99a854b9c3e1db7d79525f.zip"],
)

http_archive(
    name = "net_zstd",
    build_file = "@com_google_riegeli//third_party:net_zstd.BUILD",
    sha256 = "b6c537b53356a3af3ca3e621457751fa9a6ba96daf3aebb3526ae0f610863532",
    strip_prefix = "zstd-1.4.5/lib",
    urls = ["https://github.com/facebook/zstd/archive/v1.4.5.zip"],
)

http_archive(
    name = "snappy",
    build_file = "@com_google_riegeli//third_party:snappy.BUILD",
    sha256 = "e170ce0def2c71d0403f5cda61d6e2743373f9480124bcfcd0fa9b3299d428d9",
    strip_prefix = "snappy-1.1.9",
    urls = ["https://github.com/google/snappy/archive/1.1.9.zip"],
)

# cel-cpp
http_archive(
    name = "com_google_cel_cpp",
    sha256 = "67fafc14e523c75e74b1706d24e5b064f1eeefb9fe3a10704709343728ac07e6",
    strip_prefix = "cel-cpp-0.7.0",
    url = "https://github.com/google/cel-cpp/archive/refs/tags/v0.7.0.tar.gz",
)
