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
        TestHost.cpp
        # Test subjects (add host src files here as they are created)
        # Ex: ../src/Protocol.cpp
        # Ex: ../src/FirmwarePackage.cpp
)

# Include host src directory once it exists
target_include_directories(TestRunnerHost PRIVATE
    # ../src
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunnerHost gmock_main)
