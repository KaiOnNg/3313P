#Client.py
import socket
import sys

def main():
    # Create a socket object
    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Server information
    server_address = ('localhost', 12345)  # Server IP and port

    # Connect to the server
    try:
        sockfd.connect(server_address)
        print("Connected to the server.")
    except Exception as e:
        print("Connection to the server failed:", e)
        return

    while True:
        try:
            # Receive data from the server
            data = sockfd.recv(1024)

            sockfd.sendall("ack".encode())

            if not data:
                print("No response from server, or connection closed.")
                break

            received_msg = data.decode().strip()

            if received_msg == "Bye":
                print(received_msg)
                break

            print(received_msg)

            # React to specific server prompts
            if received_msg in ["Do you want to hit or stand?", 
                                "Do you want to continue playing? (yes or no)",
                                "which rooom do you want to join in 1 or 2 or 3",
                                "Invalid input, please try again."]:
                user_input = input("> ")  # Get input from the user

                # Send input to the server
                sockfd.sendall(user_input.encode())
            
        except KeyboardInterrupt:
            print("Keyboard interrupt detected. Exiting.")
            break
        except Exception as e:
            print("An error occurred:", e)
            break

    # Close the socket
    sockfd.close()
    sys.exit(0)

if __name__ == "__main__":
    main()