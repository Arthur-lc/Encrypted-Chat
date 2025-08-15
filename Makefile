all: check-openssl
	cd client && $(MAKE)
	cd server && $(MAKE)

check-openssl:
	@echo "Checking OpenSSL installation..."
	@if ! pkg-config --exists openssl; then \
		echo "OpenSSL not found. Please install OpenSSL development libraries:"; \
		echo "  Ubuntu/Debian: sudo apt-get install libssl-dev"; \
		echo "  CentOS/RHEL: sudo yum install openssl-devel"; \
		echo "  macOS: brew install openssl"; \
		exit 1; \
	fi
	@echo "OpenSSL found. Proceeding with build..."

clean:
	cd client && $(MAKE) clean
	cd server && $(MAKE) clean