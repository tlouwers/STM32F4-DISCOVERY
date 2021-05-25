# The executable, starting point for the unit tests
add_executable(TestRunner TestRunner.cpp)

set(CMAKE_GENERATOR "Ninja")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")

# List of the sources and unit-test files
target_sources(TestRunner
    PRIVATE
        # Unit test files
        TestRunner.cpp
        TestDummyClass.cpp
        # Mocks and Fakes
        #   Fake/...
        # Test subjects
        ../target/Src/DummyClass.cpp
        # Used sources (not part of unit tests)
        # Ex: ../target/Src/utility/crc32.cpp
)

# Include the real and Fake source directories
target_include_directories(TestRunner PRIVATE
    ../target/Src
    ./Fake
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
