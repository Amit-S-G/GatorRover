import socket
import time
import random

SERVER_IP = "127.0.0.1"   
SERVER_PORT = 8080    

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((SERVER_IP, SERVER_PORT))
print(f"🤝 Connected to Car Server at {SERVER_IP}:{SERVER_PORT}")

try:
    while True:
        left_voltage = random.randint(0, 4095)
        right_voltage = random.randint(0, 4095)

        msg = f"L:{left_voltage} R:{right_voltage}\n"

        client.send(msg.encode())

        print(f"Sent: {msg.strip()}")

        time.sleep(2)  
except KeyboardInterrupt:
    print("Client stopped.")
finally:
    client.close()
