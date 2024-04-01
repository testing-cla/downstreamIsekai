"""OMNeT++ deps rules"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel:omnetpp_bzl_version.bzl", "omnetpp_bazel_version")

def omnetpp_deps_step_1():
    """Uses rules_foreign_cc to build the external deps of omnetpp.
    """

    omnetpp_bazel_version()

    if not native.existing_rule("rules_foreign_cc"):
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "69023642d5781c68911beda769f91fcbc8ca48711db935a75da7f6536b65047f",
            strip_prefix = "rules_foreign_cc-0.6.0",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.6.0.tar.gz",
        )
