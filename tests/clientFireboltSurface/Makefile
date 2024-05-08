# Define compiler and linker flags (adjust as needed)
 CXXFLAGS = -std=c++11 -Wall
 LDFLAGS = -lwayland-client

# Source files for the executable
CLIENT_SOURCES = clientFirebolt.cpp protocol/firebolt_surface_protocol.c

 # Target executable name (override from CMake)
 CLIENT_TARGET = firebolt_extension_test

# Define the executable build rule
all: $(CLIENT_TARGET)

 $(CLIENT_TARGET): $(CLIENT_SOURCES)
     $(CXX) $(CXXFLAGS) -o $@ $(CLIENT_SOURCES) $(LDFLAGS)

# Clean rule to remove the executable
clean:
	rm -f $(CLIENT_TARGET)
