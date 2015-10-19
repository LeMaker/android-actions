package(
    default_visibility = [
        "//java/com/google/android/apps/bigtop:__subpackages__",
    ],
)

android_manifest_merge(
    name = "merged_manifest",
    srcs = ["AndroidManifest.xml"],
    deps = [
        "//java/com/google/android/apps/bigtop/third_party/bitmap:AndroidManifest.xml",
    ],
)

android_resources(
    name = "resources",
    custom_package = "com.example.bitmapsample",
    inline_constants = 0,
    manifest = ":merged_manifest",
    resources = glob(["res/**"]) + [
        "//java/com/google/android/apps/bigtop/third_party/bitmap:resource_files",
    ],
    resources_dir = "res",
    visibility = ["//visibility:private"],
)

exports_files([
    "proguard-config.pro",
])

android_binary(
    name = "bitmap_sample",
    srcs = glob(["src/**/*.java"]) + [":resources"],
    proguard_specs = [
        ":proguard-config.pro",
    ],
    resources = ":resources",
    deps = [
        "//java/com/google/android/apps/bigtop/third_party/bitmap",
    ],
)
