"""Solves the bazel version issue when building OMNeT++."""

def _impl(repository_ctx):
    repository_ctx.file("bazel_version.bzl", "bazel_version='" + native.bazel_version + "'")
    repository_ctx.file("def.bzl", "BAZEL_VERSION='" + native.bazel_version + "'")
    repository_ctx.file(
        "BUILD",
        "exports_files(['bazel_version.bzl', 'def.bzl'])",
    )

def omnetpp_bazel_version():
    if not native.existing_rule("bazel_version"):
        _bazel_version_repository = repository_rule(
            implementation = _impl,
            local = True,
        )
        _bazel_version_repository(name = "bazel_version")
