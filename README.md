# OCTGui

Control, data acquisition, post-processing, and visualization all-in-one GUI for a rotary pullback OCT catheter system in the Optical and Ultrasound Imaging Lab.

Qt6 is the cross-platform GUI framework. Data acquisition with the AlazarTech SDK is currently only supported on Windows, but it should be trivial to add support for Linux (where AlazarTech SDK is available).

## Building

Refer to the [GitHub action script](./.github/workflows/build.yml) for exact instructions on how to build the executable on Windows and macOS. Otherwise, rough instructions are given below.

This project is based on [kwsp/CppTemplate](https://github.com/kwsp/CppTemplate), which uses CMake + Ninja as the build system, VCPKG as the package manager, and Clang as the preferred compiler, with build presets for x64 Windows and arm64 macOS defined in `CMakePresets.json`. OCTGui will probably build on other platforms too but its not tested.

### 1. Install a compiler and build system

#### Windows

Install Visual Studio, and in the installer, select "Desktop development with C++", and in individual components, make sure the latest MSVC compiler, CMake, Ninja, and optionally Clang is selected. Ninja builds more than twice as fast as MSBuild so it is preferred.

#### macOS

Install Xcode or command line tools for the Apple Clang compiler. Last I checked, Qt didn't build with Homebrew Clang.

Install the [homebrew package manager](https://brew.sh/), then run the following command to install mono (a C# runtime required by VCPKG's NuGet integration), ninja (build system), llvm (for clang-tidy), and some other tools required to build the dependencies.

```sh
brew install mono ninja llvm
ln -s $(brew --prefix llvm)/bin/clang-tidy /usr/local/bin/clang-tidy
brew install autoconf autoconf-archive automake libtool
```

### 2. Install the VCPKG package manager

Follow the instructions here: <https://github.com/microsoft/vcpkg>. On macOS, clone the repo to `~/vcpkg`, and on Windows, clone the repo to `C:/vcpkg`.

### 3. Configure the project

In CMake lingo this means CMake build's the build system (Ninja); with VCPKG integration, this step takes ~30min on the first run because it also builds the dependencies defined in `vcpkg.json`. On subsequent runs, the prebuilt binaries of the dependencies are cached.

#### Windows

Activate the Visual Studio Developer PowerShell and run all commands inside the Developer PowerShell

3 CMake presets are provided for Windows

1. `clang-cl`: (preferred) x64 ClangCL + Ninja Multi-config
2. `cl`: x64 CL + Ninja Multi-config
3. `win64`: x64 CL + MSBuild

```powershell
cmake --preset clang-cl
```

#### macOS

`clang` is defined for Clang + Ninja Multi-Config

```sh
cmake --preset clang
```

### 4. Build the project

CMake build presets are defined with the `-debug`, `-relwithdebinfo`, and `-release` for "debug", "release with debug info", and "release" builds. For development, prefer `relwithdebinfo` unless something can't be debugged in this build.

#### Windows

```powershell
cmake --build --preset clang-cl-relwithdebinfo
```

#### macOS

```sh
cmake --build --preset clang-relwithdebinfo
```

## Development

I mainly develop with VS Code with the clangd and CMake Tools extensions.

After configuring the project, copy (or symlink if on \*nix) `compile_commands.json` from the build directory into the root directory and clangd will pick it up automatically.
