# SHA512 is the hash of the GitHub archive for tag ${VERSION}. After tagging a release,
# set it to 0, run `vcpkg install nanoid-cpp --overlay-ports=<nanoid_cpp>/ports` from a
# folder outside this repository (inside it, vcpkg uses the root vcpkg.json instead),
# and copy the hash from the error message.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mykolamor/nanoid_cpp
    REF "${VERSION}"
    SHA512 15303e4f527d2549c8b130343ca58fcd7277e053f7913b0ee60ca07f8636f00bfaa1d3a2e6596c034ec2477ffcd2e75990b94cc37acaa96057ef6589dad87dfe
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNANOID_BUILD_TESTS=OFF
        -DNANOID_BUILD_EXAMPLES=OFF
        -DNANOID_BUILD_CLI=OFF
        -DNANOID_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME nanoid CONFIG_PATH "lib/cmake/nanoid")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
