#!/bin/bash

# Demo script for TCP Character Counter Server
# Builds, starts server, and runs test client

echo "=== TCP Character Counter Demo ==="
echo ""

# Build the project
echo "Building project..."
./build.sh
if [ $? -ne 0 ]; then
    echo "Build failed. Exiting."
    exit 1
fi

echo ""
echo "Starting server in background..."
./run_server.sh &
SERVER_PID=$!

# Wait a moment for server to start
sleep 2

echo "Running test client..."
echo ""

# Test valid inputs
echo "Test 1: 'hello'"
./run_client.sh 127.0.0.1 5050 hello
echo ""

echo "Test 2: 'programming'"  
./run_client.sh 127.0.0.1 5050 programming
echo ""

echo "Test 3: 'aabbcc'"
./run_client.sh 127.0.0.1 5050 aabbcc
echo ""

# Test invalid input
echo "Test 4: 'hello123' (should fail)"
./run_client.sh 127.0.0.1 5050 hello123
echo ""

echo "Demo complete. Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "Demo finished."