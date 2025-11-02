# fake server
import socket

HOST = "127.0.0.1"   
PORT = 8080          

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(1)
print(f"Server listening on {HOST}:{PORT} ...")

conn, addr = server_socket.accept()
print(f"Connected by {addr}")

try:
    while True:
        data = conn.recv(1024).decode().strip()
        if not data:
            break
        print(f"Received: {data}")

        if "L:" in data and "R:" in data:
            parts = data.split()
            try:
                L = int(parts[0].split(":")[1])
                R = int(parts[1].split(":")[1])
            except Exception:
                print("Wrong data format")
                continue
            forwardThreshold = 30
            backwardThreshold = 1100

            if L <= forwardThreshold:
                print("Left Wheel: forwards")
            elif L >= backwardThreshold:
                print("Left Wheel: backwards")
            else:
                print("Left Wheel: stop")

            if R <= forwardThreshold:
                print("Right Wheel: forwards")
            elif R >= backwardThreshold:
                print("Right Wheel: backwards")
            else:
                print("Right Wheel: stop")

except KeyboardInterrupt:
    print("Server manually stopped.")
finally:
    conn.close()
    server_socket.close()
