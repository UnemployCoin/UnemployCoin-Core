 #!/usr/bin/env bash

 # Execute this file to install the unemploycoin cli tools into your path on OS X

 CURRENT_LOC="$( cd "$(dirname "$0")" ; pwd -P )"
 LOCATION=${CURRENT_LOC%UnemployCoin-Qt.app*}

 # Ensure that the directory to symlink to exists
 sudo mkdir -p /usr/local/bin

 # Create symlinks to the cli tools
 sudo ln -s ${LOCATION}/UnemployCoin-Qt.app/Contents/MacOS/unemploycoind /usr/local/bin/unemploycoind
 sudo ln -s ${LOCATION}/UnemployCoin-Qt.app/Contents/MacOS/unemploycoin-cli /usr/local/bin/unemploycoin-cli
