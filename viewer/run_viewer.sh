#!/bin/bash

# make clean
# make

ENCODER_PATH="tcp_module"

# echo "Downloading files"
# ./tcp_module/vid_downloader 13.90.224.167 41231 $ENCODER_PATH/encoder_cert.der $ENCODER_PATH/output.h264 $ENCODER_PATH/metadata.json $ENCODER_PATH/output.sig
./tcp_module/vid_downloader 20.39.52.2 41231 $ENCODER_PATH/encoder_cert.der $ENCODER_PATH/output.h264 $ENCODER_PATH/metadata.json $ENCODER_PATH/output.sig

echo "Verifying (and displaying)"
./verification_with_gui.py $ENCODER_PATH/output.h264 $ENCODER_PATH/output.sig $ENCODER_PATH/encoder_cert.der $ENCODER_PATH/metadata.json

# echo "Verifying signature"
# ./sig_verify $ENCODER_PATH/output.h264 $ENCODER_PATH/output.sig $ENCODER_PATH/encoder_cert.der $ENCODER_PATH/metadata.json

# echo "Displaying video"
# vlc $ENCODER_PATH/output.h264
