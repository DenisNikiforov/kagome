## Template for a custom hunter configuration
# Useful when there is a need for a non-default version or arguments of a dependency,
# or when a project not registered in soramitsu-hunter should be added.
#
# hunter_config(
#     package-name
#     VERSION 0.0.0-package-version
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )
#
# hunter_config(
#     package-name
#     URL https://repo/archive.zip
#     SHA1 1234567890abcdef1234567890abcdef12345678
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )

hunter_config(
    backward-cpp
    URL https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.zip
    SHA1 93c4c843fc9308e62ac462459077d87dc6dd9885
    CMAKE_ARGS BACKWARD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    soralog
    VERSION 0.0.9
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/refs/heads/feature/cmake-config.tar.gz
    SHA1 ad6fc09c643d0a9cb64f3d35960e05bbc6ed9c6d
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    schnorrkel_crust
    URL https://github.com/soramitsu/soramitsu-sr25519-crust/archive/refs/heads/refactor/cmake-config.tar.gz
    SHA1 4667a0b1d564ee05d224425139b30755b0401926
    KEEP_PACKAGE_SOURCES
)

hunter_config(
  wavm
  URL https://github.com/soramitsu/WAVM/archive/7efbcced0d41d5f7bc6cd254d624e5f7174b54fc.tar.gz
  SHA1 dd22c11c5faf95a6f6b05ecff18f0e8cab475771
  CMAKE_ARGS
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
  KEEP_PACKAGE_SOURCES
)
