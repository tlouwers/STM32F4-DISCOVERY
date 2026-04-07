# Coding Style Guide

This document defines the coding style for all C++ source files in this project.
It is derived from the style used in the `drivers/` folder of the STM32F4-DISCOVERY repository
and must be followed consistently across `host/src/`, `host/tests/`, firmware modules, and shared utilities.

---

## 1. File Header

Every `.hpp` and `.cpp` file begins with a Doxygen file header block using the Beer-Ware licence.

**Interface / header file (`.hpp`):**

```cpp
/**
 * \file    ISerial.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for serial port communication.
 *
 * \details Implementations handle OS-specific serial I/O. The ST AN3155
 *          bootloader protocol requires 8E1 (8 data bits, even parity, 1 stop bit).
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */
```

**Class implementation file (`.cpp`):**

```cpp
/**
 * \file    SerialPosix.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SerialPosix
 *
 * \brief   Linux/POSIX serial port implementation using termios.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */
```

**Tags used:**

| Tag | When required | Notes |
|---|---|---|
| `\file` | Always | Filename only, no path |
| `\licence` | Always | Beer-Ware text verbatim |
| `\class` | `.cpp` files only | Class name |
| `\brief` | Always | One-line summary |
| `\details` | When useful | Multi-line elaboration |
| `\note` | Always | GitHub URL to the directory |
| `\author` | Always | Full name + email |
| `\version` | Always | Start at `1.0` |
| `\date` | Always | `MM-YYYY` |

---

## 2. Include Guards

Use `#ifndef` / `#define` / `#endif` guards. The macro name is the filename in `SCREAMING_SNAKE_CASE` with a trailing underscore.

```cpp
#ifndef SERIAL_POSIX_HPP_
#define SERIAL_POSIX_HPP_

// ... content ...

#endif  // SERIAL_POSIX_HPP_
```

The `#endif` comment must match the guard macro exactly.

---

## 3. Section Banners

Divide every file into sections using an 72-character banner comment. Use consistent section names.

```cpp
/************************************************************************/
/* Includes                                                             */
/************************************************************************/

/************************************************************************/
/* Enums                                                                */
/************************************************************************/

/************************************************************************/
/* Structs                                                              */
/************************************************************************/

/************************************************************************/
/* Constants                                                            */
/************************************************************************/

/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/

/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/

/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/

/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
```

Use only the sections that are actually needed in a given file. Maintain the order listed above.

---

## 4. Naming Conventions

### Classes and Interfaces

- **PascalCase** for all class and interface names.
- Interface classes use the `I` prefix: `ISerial`, `ICrc`.
- Concrete classes do **not** use a prefix: `SerialPosix`, `SoftwareCrc32`.
- Mark concrete (non-inheritable) classes `final`.

```cpp
class SerialPosix final : public ISerial { ... };
```

### Methods

- **PascalCase** for all public and private member functions.

```cpp
bool Open(const std::string& port, const SerialConfig& config);
void Close();
bool IsOpen() const;
int  Write(const uint8_t* data, size_t length);
int  Read(uint8_t* buffer, size_t length);
void SetTimeout(uint32_t timeoutMs);
void FlushInput();
uint32_t Compute(const uint8_t* data, size_t length) const;
```

### Member Variables

- **`m`-prefix, camelCase** — no underscore separator.

```cpp
int      mFd;
uint32_t mTimeoutMs;
bool     mInitialized;
bool     mOpen = false;
std::array<uint32_t, 256> mTable;
```

### Local Variables and Parameters

- **camelCase**, no prefix.

```cpp
size_t totalWritten = 0;
uint32_t word = 0;
speed_t speed;
```

### Constants

- **`k`-prefix, PascalCase** for `static constexpr` values.

```cpp
static constexpr uint32_t kPolynomial = 0x04C11DB7u;
static constexpr uint32_t kInitValue  = 0xFFFFFFFFu;
```

### Enums

- **PascalCase** for the enum type name.
- **PascalCase** for enumerator values.

```cpp
enum class Parity : uint8_t
{
    None,
    Even,
    Odd
};
```

### Files

- Class files: `ClassName.hpp` / `ClassName.cpp` — PascalCase, matching the class name.
- Interface files: `IClassName.hpp` — same rule with `I` prefix.
- Test files: `TestSubjectName.cpp`.
- Mock files: `MockClassName.hpp`.

---

## 5. Includes

### Order

1. Own header (in `.cpp` files) — always first, using the full path from `src/`
2. OS / system headers (angle-bracket)
3. Standard library headers (angle-bracket)

Separate each group with a blank line.

```cpp
#include "serial/SerialPosix.hpp"    // own header, full path from src/

#include <fcntl.h>                   // OS headers
#include <termios.h>
#include <unistd.h>

#include <cstdint>                   // standard library
#include <string>
```

### Include paths

Always use **full paths relative to the `src/` root**, not relative `../` paths or bare filename includes:

```cpp
// Correct
#include "crc/SoftwareCrc32.hpp"
#include "serial/ISerial.hpp"

// Wrong
#include "SoftwareCrc32.hpp"
#include "../crc/SoftwareCrc32.hpp"
```

---

## 6. Class Layout

Declare class members in this order:

1. `public:` — constructors, destructor, then methods
2. `private:` — member variables, then private methods

```cpp
class SerialPosix final : public ISerial
{
public:
    SerialPosix();
    virtual ~SerialPosix();

    bool Open(const std::string& port, const SerialConfig& config) override;
    void Close() override;
    bool IsOpen() const override;

    int  Write(const uint8_t* data, size_t length) override;
    int  Read(uint8_t* buffer, size_t length) override;

    void SetTimeout(uint32_t timeoutMs) override;
    void FlushInput() override;

private:
    int      mFd;
    uint32_t mTimeoutMs;

    bool ApplyConfig(const SerialConfig& config);
};
```

- One blank line between method declarations in the header.
- Group related methods together.
- Member variables are declared before private methods.

---

## 7. Doxygen Method Comments

Every **public** method in a `.cpp` file has a Doxygen comment block immediately above it.

```cpp
/**
 * \brief   Opens a serial port with the given configuration.
 * \param   port    Device path (e.g. "/dev/ttyUSB0").
 * \param   config  Baud rate, parity, stop bits, and timeout settings.
 * \returns True if the port was opened and configured successfully.
 */
bool SerialPosix::Open(const std::string& port, const SerialConfig& config)
{
```

**Tags:**

| Tag | Use |
|---|---|
| `\brief` | Always — one-line summary |
| `\param` | One per parameter; align descriptions with spaces |
| `\returns` | When the return value is non-void |
| `\note` | Optional — important caveats |

Private methods have a `\brief` at minimum. Simple setters/getters may use a single-line `\brief` only.

---

## 8. Braces and Formatting

- **Allman style** — opening brace on its own line.
- Indent with **4 spaces** — no tabs.
- Always use braces, even for single-statement `if`/`for`/`while` bodies.

```cpp
if (mFd >= 0)
{
    ::close(mFd);
    mFd = -1;
}
```

Exception: `switch` cases with a single `break` may omit braces; `case` labels are not indented relative to `switch`.

```cpp
switch (config.parity)
{
    case Parity::None:
        tty.c_cflag &= ~PARENB;
        break;
    case Parity::Even:
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
        break;
}
```

---

## 9. Inline Comments

- Use `//` for inline comments, not `/* */`.
- Place a comment on its own line above the code it describes, or aligned to the right with spacing.
- Comments explain **why**, not **what** — the code should be self-explanatory for the *what*.

```cpp
// Clear non-blocking after open (we use select for timeout)
int flags = fcntl(mFd, F_GETFL, 0);

tcdrain(mFd);    // wait for output to drain before returning
```

---

## 10. C++ Standard and Language Rules

- **C++17** for all host-side (`host/src/`, `host/tests/`) code.
- **C++14** for firmware (cross-compiled) code where toolchain compatibility requires it.
- `nullptr` — never `NULL` or `0` for pointers.
- `static_cast<>` — never C-style casts.
- `constexpr` — preferred over `#define` for constants.
- `enum class` — always, never plain `enum`.
- `override` — always on overriding methods.
- `final` — always on leaf classes.
- No raw owning pointers in new code — use references or smart pointers.
- No `using namespace std;` at file scope.

---

## 11. Unit Test Style

Test files follow the Google Test naming convention.

```cpp
class ClassName_Test : public ::testing::Test
{
protected:
    SubjectClass mSubject;   // use m-prefix for member instances
};

TEST_F(ClassName_Test, MethodName_Condition_ExpectedResult)
{
    // Arrange
    const uint8_t data[] = { 0x12, 0x34, 0x56, 0x78 };

    // Act / Assert
    EXPECT_EQ(mSubject.Compute(data, sizeof(data)), 0xDF8A8A2Bu);
}
```

- Test name format: `MethodName_Condition_ExpectedResult` (underscores as separators).
- One `EXPECT_*` or `ASSERT_*` per logical check where practical.
- Prefer `EXPECT_*` over `ASSERT_*` unless the test cannot continue after failure.
- Mock files are named `MockClassName.hpp` and live in `host/tests/`.

---

## 12. CMakeLists Style

Follow the pattern established in `ExampleProject`:

- Root `CMakeLists.txt`: project definition + GoogleTest + `add_subdirectory(tests)`. No library targets.
- `tests/CMakeLists.txt`: a single `TestRunner` executable. List all test subjects directly as sources (no intermediate static library).
- Add new source files to `target_sources(TestRunnerHost PRIVATE ...)` as they are created.
- Keep commented-out examples for the next developer:

```cmake
target_sources(TestRunnerHost
    PRIVATE
        TestRunner.cpp
        TestCrc32.cpp
        # Test subjects
        ../src/crc/SoftwareCrc32.cpp
        # Ex: ../src/transport/UsartTransport.cpp
)
```
