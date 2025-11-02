import socket
import time
import random

SERVER_IP = "192.168.4.1"
SERVER_PORT = 8080

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    client.connect((SERVER_IP, SERVER_PORT))
    print(f"Connected to ESP32 {SERVER_IP}:{SERVER_PORT}")
except Exception as e:
    print("Connection failed")
    print(e)
    exit()

try:
    while True:
        left_voltage = random.randint(0, 4095)
        right_voltage = random.randint(0, 4095)

        def get_action(v):
            if v <= 30:
                return "forwards"
            elif v >= 1100:
                return "backwards"
            else:
                return "stop"

        left_action = get_action(left_voltage)
        right_action = get_action(right_voltage)

        print(f"Left: {left_voltage:4d} ({left_action}) | Right: {right_voltage:4d} ({right_action})")

        msg = f"L:{left_voltage} R:{right_voltage}\n"
        client.send(msg.encode())

        time.sleep(0.5)
except KeyboardInterrupt:
    print("Client stopped.")
finally:
    client.close()
    print("Disconnected.")
