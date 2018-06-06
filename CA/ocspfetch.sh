#!/bin/sh

ocspcheck -v -N -o server.ocsp.der server.crt
if [ $? == 0 ]; then
    echo "ocsp looks ok"
fi
