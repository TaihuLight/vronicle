# # Clean and make
# make clean
# make

# # Make Encoder
# cd encoder
# ./build_x86.sh
# cd ..

# # Setup v4l2 cam (Suppose your webcam is video0)
# v4l2-ctl -d /dev/video0 -c exposure_auto=1
# v4l2-ctl -d /dev/video0 -c brightness=100
# v4l2-ctl -d /dev/video0 -c exposure_absolute=664

# # Create directory to save output
# mkdir -p output

# Set variables
NUM_OF_FRAMES="60"
VIDEO_FILE_NAME="./output/out_raw"
ENC_VIDEO_FILE_NAME="./output/out.h264"
SIG_FILE_NAME="./output/out.sig"
MD_FILE_NAME="./output/metadata.json"
CERT_FILE_NAME="./signer/camera_cert"
PRIVKEY_FILE_NAME="./signer/camera_pri"
PUBKEY_FILE_NAME="./signer/camera_pub"
# SERVER_IP_ADDR="13.90.224.167"
SERVER_IP_ADDR="20.39.52.2"
# SERVER_PORT="10111"
SERVER_PORT="10112"

# # run
# ./video_capture/take_yuyv_frames $NUM_OF_FRAMES $VIDEO_FILE_NAME

# encode
# ./encoder/h264enc_x64 -is_yuyv -fps10 -numframes$NUM_OF_FRAMES $VIDEO_FILE_NAME $ENC_VIDEO_FILE_NAME $MD_FILE_NAME $SIG_FILE_NAME $PRIVKEY_FILE_NAME $PUBKEY_FILE_NAME
# ./encoder/h264enc_x64 -fps10 -numframes$NUM_OF_FRAMES $VIDEO_FILE_NAME $ENC_VIDEO_FILE_NAME $MD_FILE_NAME $SIG_FILE_NAME $PRIVKEY_FILE_NAME $PUBKEY_FILE_NAME

# sign encoded video

chmod -x,+r $VIDEO_FILE_NAME

# Upload everything to the server

./tcp_client/client $SERVER_IP_ADDR $SERVER_PORT $MD_FILE_NAME meta $CERT_FILE_NAME cert $ENC_VIDEO_FILE_NAME vid $SIG_FILE_NAME sig

# ./tcp_client/client $SERVER_IP_ADDR $SERVER_PORT $CERT_FILE_NAME cert
# ./tcp_client/client $SERVER_IP_ADDR $SERVER_PORT $ENC_VIDEO_FILE_NAME vid
# ./tcp_client/client $SERVER_IP_ADDR $SERVER_PORT $MD_FILE_NAME meta
# ./tcp_client/client $SERVER_IP_ADDR $SERVER_PORT $SIG_FILE_NAME sig