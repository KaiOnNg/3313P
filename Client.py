#Client.py
import socket
import sys
from cffi import FFI

ffi = FFI()

ffi.cdef(
        """
    typedef struct Semaphore Semaphore;

    Semaphore* Semaphore_new(const char* name, int initialState, bool createAsOwner);
    void Semaphore_signal(Semaphore* sem);
    void Semaphore_wait(Semaphore* sem);
    void Semaphore_delete(Semaphore* sem);
    """
)

lib = ffi.dlopen("./libsemaphorewrapper.so")

def main():
    # Create a socket object
    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    sem = lib.Semaphore_new(b"socketWrite", 1, False)
    if sem == ffi.NULL:
        raise RuntimeError("Failed to create semaphore")

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
            lib.Semaphore_signal(sem)

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
    lib.Semaphore_delete(sem)
    sockfd.close()
    sys.exit(0)

if __name__ == "__main__":
    main()