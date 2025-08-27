#!/bin/bash

# Run script for TCP Character Counter Server
# Starts the server on default port 5050

SERVER_IP=${1:-127.0.0.1}
SERVER_PORT=${2:-5050}
THREAD_COUNT=${3:-200}

echo "=== Starting TCP Character Counter Server ==="
echo "Server IP: $SERVER_IP"
echo "Server Port: $SERVER_PORT"
echo "Thread Count: $THREAD_COUNT"
echo "Press Ctrl+C to stop the server"
echo ""

# Check if server executable exists
if [ ! -f "build/server/server_app" ]; then
    echo "Error: Server executable not found. Please run ./build.sh first."
    exit 1
fi

# Start the server
./build/server/server_app $SERVER_IP $SERVER_PORT $THREAD_COUNT
