#!/bin/bash

export BUILD_TYPE=release
export PROXY_PROFILE=release
if [[ $(uname -s) == "Darwin" ]]; then
  echo "Kernel is Darwin, change build type to debug"
  unset BUILD_TYPE
  export PROXY_PROFILE=debug
  make build_by_type
  mkdir -p target/release
  cp target/debug/libtiflash_proxy.dylib target/release/libtiflash_proxy.dylib
else
  make build_by_type
fi