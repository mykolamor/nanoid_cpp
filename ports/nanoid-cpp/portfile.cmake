# SHA512 is the hash of the GitHub archive for tag ${VERSION}. After tagging a release,
# set it to 0, run `vcpkg install nanoid-cpp --overlay-ports=<nanoid_cpp>/ports` from a
# folder outside this repository (inside it, vcpkg uses the root vcpkg.json instead),
# and copy the hash from the error message.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mykolamor/nanoid_cpp
    REF "${VERSION}"
    SHA512 81588b49ac19d7d5bf7f1721999235839c5415a595d909a9e2b6a7f80b8a0039bbfddfc782648d66f18b0f857e5999826884c7db3d8dc9264e6faea9077e7778
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNANOID_BUILD_TESTS=OFF
        -DNANOID_BUILD_EXAMPLES=OFF
        -DNANOID_BUILD_CLI=OFF
        -DNANOID_INSTALL=ON
    MAYBE_UNUSED_VARIABLES
        NANOID_BUILD_CLI
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME nanoid CONFIG_PATH "lib/cmake/nanoid")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
