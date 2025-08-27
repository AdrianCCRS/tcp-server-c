#!/bin/bash

# Run script for TCP Character Counter Client
# Usage: ./run_client.sh <server_ip> <server_port> <text>

if [ $# -ne 3 ]; then
    echo "Usage: $0 <server_ip> <server_port> <text>"
    echo "Example: $0 127.0.0.1 5050 hello"
    echo ""
    echo "Text must contain only alphabet characters (a-z, A-Z)"
    exit 1
fi

SERVER_IP=$1
SERVER_PORT=$2
TEXT=$3

echo "=== TCP Character Counter Client ==="
echo "Server: $SERVER_IP:$SERVER_PORT"
echo "Text: '$TEXT'"
echo ""

# Check if client executable exists
if [ ! -f "build/client/vowel_client" ]; then
    echo "Error: Client executable not found. Please run ./build.sh first."
    exit 1
fi

# Run the client
./build/client/vowel_client "$SERVER_IP" "$SERVER_PORT" "$TEXT"