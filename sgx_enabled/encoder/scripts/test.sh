_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

./h264enc_x86 vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

./h264enc_x86_sse2 vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

./h264enc_x64 vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

qemu-arm ./h264enc_arm_gcc vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

qemu-arm ./h264enc_arm_gcc_asm vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

qemu-aarch64 ./h264enc_arm64_gcc vectors/foreman.cif
if ! cmp ./out.264 vectors/out_ref.264 >/dev/null 2>&1
then
    echo test failed
    exit 1
fi
rm out.264

echo test passed
