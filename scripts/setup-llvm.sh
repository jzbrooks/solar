set -ex

SOLAR_DIR=$PWD

LLVM_VERSION=11.1.0
LLVM_URL=https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/llvm-$LLVM_VERSION.src.tar.xz

if [ -z "$LLVM_TARGETS" ]
then
    LLVM_TARGETS='AArch64'
fi

if [[ ! -f llvm-$LLVM_VERSION.src.tar.xz ]]
then
    wget $LLVM_URL || curl --output llvm-$LLVM_VERSION.src.tar.xz $LLVM_URL
fi
if [[ ! -d llvm-$LLVM_VERSION.src ]]
then
    tar -vxzf llvm-$LLVM_VERSION.src.tar.xz
fi

cd llvm-$LLVM_VERSION.src
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_TARGETS_TO_BUILD=$LLVM_TARGETS -DLLVM_ENABLE_DUMP=ON -DCMAKE_INSTALL_PREFIX=$SOLAR_DIR/llvm
cmake --build . --target install -- -j

rm llvm-$LLVM_VERSION.src.tar.xz
rm -rf llvm-$LLVM_VERSION.src/

cd "$SOLAR_DIR"
