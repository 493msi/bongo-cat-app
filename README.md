# bongo-cat-app

A desktop BongoCat mouse and keyboard visualizer for Windows.  

Bongo cat art courtesy of [@StrayRogue](https://twitter.com/StrayRogue).

## Source License

Public Domain. See [LICENSE](LICENSE) for details.

## Requirements

* Windows 10+, sorry
* CMake

### MSVC

* Visual Studio 2019
* Tested with vcpkg

### MinGW

* Msys2 + MinGW-w64
* GCC supporting C++14
* GNU Make

## Building

### MSVC

Install [vcpkg](https://vcpkg.io/en/getting-started.html) and install the following packages: 

* `sdl2:x64-windows`
* `sdl2-image:x64-windows`

In Developer PowerShell for VS 2019 (x64):

```powershell
mkdir build-msvc
cd build-msvc
cmake -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release -A x64 ..
msbuild .\bongo-cat-app.sln
```

### Msys2 + MinGW

Install `SDL2` and `SDL2_image` from the Msys2 package manager.

```shell
mkdir build && cd build
cmake -G "Unix Makefiles" ..
make
```

## Running

It is important to run the application from the root project directory,
as it expects the `assets` directory to be in the current working directory.

### MSVC

```powershell
# Build the application
mkdir build-msvc
cd build-msvc
cmake -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release -A x64 ..
msbuild .\bongo-cat-app.sln
# Run
cd ..
build-msvc\Release\bongo-cat-app.exe
```

### Msy2 + MinGW

```shell
# Build the application
mkdir build && cd build
cmake -G "Unix Makefiles" ..
make
# Run
./build/bongo-cat-app
```