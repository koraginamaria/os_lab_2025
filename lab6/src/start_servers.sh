#!/bin/bash

echo "=== Starting Distributed Factorial System ==="

# Компилируем
make clean
make all

echo "✓ Programs compiled"

# Проверяем файл servers.txt
if [ ! -f servers.txt ]; then
    echo "Creating servers.txt..."
    cat > servers.txt << SERVERS
127.0.0.1:20001
127.0.0.1:20002
127.0.0.1:20003
SERVERS
fi

echo "Starting servers..."

# Функция для проверки порта
check_port() {
    local port=$1
    if ss -tulpn | grep ":$port" > /dev/null 2>&1; then
        return 0  # порт занят
    else
        return 1  # порт свободен
    fi
}

# Запускаем серверы
for port in 20001 20002 20003; do
    if check_port $port; then
        echo "✗ Port $port is already in use, killing process..."
        sudo fuser -k $port/tcp 2>/dev/null
        sleep 1
    fi
    
    echo "Starting server on port $port..."
    ./server --port $port --tnum 2 &
    sleep 1
    
    if check_port $port; then
        echo "✓ Server on port $port started successfully"
    else
        echo "✗ Failed to start server on port $port"
    fi
done

echo ""
echo "Waiting 2 seconds for servers to initialize..."
sleep 2

echo ""
echo "=== Testing Client ==="
./client --k 20 --mod 100000 --servers servers.txt

echo ""
echo "=== System Test Complete ==="
echo "Servers are still running in background."
echo "You can run more tests: ./client --k 50 --mod 1000 --servers servers.txt"
echo "To stop servers: pkill server"
