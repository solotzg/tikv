#!/bin/bash

set -e
export BUILD_TYPE=release
export PROXY_PROFILE=release
if [[ $(uname -s) == "Darwin" ]]; then
  echo "Kernel is Darwin, change build type to debug"
  unset BUILD_TYPE
  export PROXY_PROFILE=debug
  brew --prefix openssl
#  export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
#  export OPENSSL_LIB_DIR=$(brew --prefix openssl)"/lib"
#  export OPENSSL_INCLUDE_DIR=$(brew --prefix openssl)"/include"
#  export OPENSSL_NO_VENDOR=1
  make build_by_type
  mkdir -p target/release
  cp target/debug/libraftstore_proxy.dylib target/release/libraftstore_proxy.dylib
  sys_darwin=1
else
  make build_by_type
fi

base_path=$(cd $(dirname $0); pwd)
deploy_tag=${GRAFANA_DEPLOY_TAG}
if [ -z ${sys_darwin} ]; then
  NPROC=${NPROC:-$(nproc || grep -c ^processor /proc/cpuinfo)}
else
  NPROC=${NPROC:2}
fi
git submodule update --init --recursive
cd demo/third-party/kvproto
if [[ ! -d cpp/kvproto || -d proto-cpp ]]; then
  ./scripts/generate_cpp.sh
else
  echo "skip generate kvproto cpp"
fi
cd ../../libs
rm -rf ./raft-store-proxy && ln -s ../../target/release/ ./raft-store-proxy
cd ../
if [ ! -d build ]; then rm -rf build && mkdir build; fi
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
make -j ${NPROC}
cd ../
mkdir -p bin && cd bin
cp ../build/RaftStoreDemo ./

if [ -z ${sys_darwin} ]; then
  cp ../../target/release/libraftstore_proxy.so ./
  chrpath -d libraftstore_proxy.so RaftStoreDemo
  ldd RaftStoreDemo | grep 'ssl' | awk '{printf "\n[WARN] %s is required, may need to install related lib if copy binary to other machine\n",$1}'
else
  cp ../../target/release/libraftstore_proxy.dylib ./
  FILE="./RaftStoreDemo"
  otool -L ${FILE} | egrep -v "$(otool -D ${FILE})" | egrep -v "/(usr/lib|System)" | grep -o "/.*\.dylib" | while read; do
    install_name_tool -change $REPLY @executable_path/"$(basename ${REPLY})" $FILE;
  done
fi

cd ../third-party/client-go
GO111MODULE=on go build ./examples/test-engine/test-engine-write.go
GO111MODULE=on go build ./examples/test-tidb/test-tidb.go
GO111MODULE=on go build ./examples/test-placement-rule/test-placement-rule.go
mv test-* ../../bin

cd "${base_path}"
cp ./metrics/grafana/tikv_summary.json ./demo/bin/
cp ./metrics/grafana/tikv_details.json ./demo/bin/
cp ./demo/run-pd-client.py ./demo/bin/

cd ./demo/bin/

python3 ../replace-grafana.py --files "${base_path}/metrics/grafana/tikv_summary.json,${base_path}/metrics/grafana/tikv_details.json" --output "./" --deploy-tag "${deploy_tag}" --prometheus_metric_name_prefix ${PROMETHEUS_METRIC_NAME_PREFIX} --engine_label_value ${ENGINE_LABEL_VALUE}

mv tikv_summary.json ${ENGINE_LABEL_VALUE}_summary.json
mv tikv_details.json ${ENGINE_LABEL_VALUE}_details.json