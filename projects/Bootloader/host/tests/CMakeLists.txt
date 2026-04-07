# The executable, starting point for the host-side unit tests
add_executable(TestRunnerHost TestRunner.cpp)

set(CMAKE_GENERATOR "Ninja")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")

# List of the sources and unit-test files
target_sources(TestRunnerHost
    PRIVATE
        # Unit test files
        TestRunner.cpp
        TestCrc32.cpp
        TestHost.cpp
        # Mocks and Fakes
        # (add fakes here as needed)
        # Test subjects
        ../src/crc/SoftwareCrc32.cpp
        ../src/serial/SerialWin.cpp
        # Used sources (not part of unit tests)
        # Ex: ../src/transport/UsartTransport.cpp
)

# Include the real and Fake/Mock source directories
target_include_directories(TestRunnerHost PRIVATE
    ../src
    ./
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunnerHost gmock_main)
