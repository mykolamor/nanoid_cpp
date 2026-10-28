# SHA512 is the hash of the GitHub archive for tag ${VERSION}. After tagging a release,
# set it to 0, run `vcpkg install nanoid-cpp --overlay-ports=<nanoid_cpp>/ports` from a
# folder outside this repository (inside it, vcpkg uses the root vcpkg.json instead),
# and copy the hash from the error message.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mykolamor/nanoid_cpp
    REF "${VERSION}"
    SHA512 20fc9cc9f87d60aff8a6cc13e0b7d0d8feb0edc1387b0b7c2f5e21e8bbf51bf3035684a2edb90f20c461b5bb5205a01d35f9d1294248355e0ca1c5ad7928f118
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNANOID_BUILD_TESTS=OFF
        -DNANOID_BUILD_EXAMPLES=OFF
        -DNANOID_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME nanoid CONFIG_PATH "lib/cmake/nanoid")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
