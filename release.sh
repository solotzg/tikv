#!/bin/bash

export BUILD_TYPE=release
export PROXY_PROFILE=release
if [[ $(uname -s) == "Darwin" ]]; then
  echo "Kernel is Darwin, change build type to debug"
  unset BUILD_TYPE
  export PROXY_PROFILE=debug
  make build_by_type
  cp target/debug/libraftstore_proxy.dylib target/release/libraftstore_proxy.dylib
else
  make build_by_type
fi
