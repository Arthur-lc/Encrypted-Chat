#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
NCURSES_VERSION="6.4"
NCURSES_URL="https://ftp.gnu.org/pub/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz"
NCURSES_DIR="third_party/ncurses-${NCURSES_VERSION}"
INSTALL_DIR="$(pwd)/build"
BUILD_DIR_NCURSES="${NCURSES_DIR}/build"

# --- Dependencies Check ---
echo "Checking for required tools (g++, curl, tar)..."
for tool in g++ curl tar; do
    if ! command -v $tool &> /dev/null; then
        echo "Error: $tool is not installed. Please install it to continue."
        exit 1
    fi
done
echo "All required tools are present."

# --- Ncurses Download and Extraction ---
if [ ! -d "$NCURSES_DIR" ]; then
    echo "Ncurses source not found. Downloading and extracting..."
    mkdir -p third_party
    curl -L $NCURSES_URL -o third_party/ncurses.tar.gz
    tar -xzf third_party/ncurses.tar.gz -C third_party/
    rm third_party/ncurses.tar.gz
    echo "Ncurses source is ready."
else
    echo "Ncurses source already exists."
fi

# --- Ncurses Static Build ---
# We build ncurses only if the final library doesn't exist yet.
if [ ! -f "${INSTALL_DIR}/lib/libncursesw.a" ]; then
    echo "Building ncurses (static)..."
    cd ${NCURSES_DIR}
    
    # Configure the build to be static, without shared libs, and with wide-character support.
    # We install it into our local project 'build' directory, not the system.
    ./configure --prefix="${INSTALL_DIR}" \
                --with-shared=no \
                --enable-static=yes \
                --with-cxx-shared=no \
                --enable-pc-files=no \
                --without-ada \
                --without-manpages \
                --without-progs \
                --without-tests \
                --with-termlib \
                --enable-widec
    
    # Compile and install
    make -j$(nproc)
    make install
    
    cd - > /dev/null
    echo "Ncurses static library built and installed locally."
else
    echo "Ncurses static library already exists."
fi

# --- Application Build ---
echo "Building the T-Chat application..."

# Create build directory if it doesn't exist
mkdir -p build

# Compile server components
g++ -c server.cpp -o build/server.o
g++ -c mainServer.cpp -o build/mainServer.o
g++ build/server.o build/mainServer.o -o build/server_executable -lpthread

# Compile client components
# Add -I to tell g++ where to find the ncurses header files we built
g++ -I"${INSTALL_DIR}/include" -I"${INSTALL_DIR}/include/ncursesw" -c client.cpp -o build/client.o
g++ -I"${INSTALL_DIR}/include" -I"${INSTALL_DIR}/include/ncursesw" -c UIManager.cpp -o build/UIManager.o
g++ -I"${INSTALL_DIR}/include" -I"${INSTALL_DIR}/include/ncursesw" -c mainClient.cpp -o build/mainClient.o

# Link client statically with our custom ncurses build
# -L tells the linker where to find library files (libncursesw.a)
# -lncursesw tells the linker to link the ncursesw library
# -static ensures it's linked statically
g++ build/client.o build/UIManager.o build/mainClient.o -o build/client_executable \
    -L"${INSTALL_DIR}/lib" \
    -lncursesw -lpthread -static


echo "Build finished!"
echo "Server executable: $(pwd)/build/server_executable"
echo "Client executable: $(pwd)/build/client_executable"
echo "To run the build script again, first clean the build with: rm -rf build"
