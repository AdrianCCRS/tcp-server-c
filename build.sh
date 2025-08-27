#!/bin/bash

# Build script for TCP Character Counter Server
# Compiles both server and client applications

echo "=== Building TCP Character Counter Server ==="

# Create build directories
echo "Creating build directories..."
mkdir -p build/server
mkdir -p build/client

# Compile server
echo "Compiling server..."
gcc -o build/server/server_app server/server.c -lpthread
if [ $? -eq 0 ]; then
    echo "✓ Server compiled successfully"
else
    echo "✗ Server compilation failed"
    exit 1
fi

# Compile client  
echo "Compiling client..."
gcc -o build/client/vowel_client client/vowel_client.c
if [ $? -eq 0 ]; then
    echo "✓ Client compiled successfully"
else
    echo "✗ Client compilation failed"
    exit 1
fi

echo ""
echo "=== Build Complete ==="
echo "Server executable: build/server/server_app"
echo "Client executable: build/client/vowel_client"
echo ""
echo "Usage:"
echo "  Start server: ./run_server.sh"
echo "  Use client:   ./build/client/vowel_client <ip> <port> <text>"