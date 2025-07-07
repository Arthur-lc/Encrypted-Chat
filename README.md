# T-Chat

A simple terminal-based chat application written in C++.

## Overview

T-Chat consists of two main components:

- `server`: A multi-threaded server that handles incoming client connections and relays messages.
- `client`: A terminal client that uses a custom-built ncurses library for its interface, allowing for a clean, portable chat experience without system-wide dependencies.

## How to Compile and Run

This project uses a simple shell script to handle all build dependencies, including fetching and statically compiling the ncurses library. This ensures the application can be built on any compatible Linux system without requiring the user to have root/sudo permissions to install development libraries.

### Prerequisites

Before you begin, ensure you have the following tools installed on your system:

- A C++ compiler (`g++`)
- `curl` (for downloading dependencies)
- `tar` (for extracting archives)

On a Debian-based system (like Ubuntu), you can install them with:
```sh
sudo apt-get update
sudo apt-get install build-essential curl
```

### 1. Make the Build Script Executable

First, you need to give the build script permission to execute.

```sh
chmod +x build.sh
```

### 2. Run the Build Script

Execute the script. It will automatically download, compile, and link all necessary components.

```sh
./build.sh
```

This will create a `build` directory containing the final executables.

### 3. Run the Server

Open a terminal and start the server. It will begin listening for client connections.

```sh
./build/server_executable
```

### 4. Run the Client

Open one or more new terminals and run the client executable to connect to the server.

```sh
./build/client_executable
```

Now you can start chatting between the connected clients.

## Cleaning the Build

If you want to perform a fresh build from scratch, you can remove the `build` and `third_party` directories:

```sh
rm -rf build third_party
```
