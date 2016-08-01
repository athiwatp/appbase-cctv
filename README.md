# appbase-cctv
A CCTV system for the IoT backed on [Appbase](https://appbase.io/).

## Dependencies
This is the list of libraries required by appbase-cctv, plus the install command for Debian (*jessie*, derivatives may also apply).
- [libcurl](https://curl.haxx.se/libcurl/c/): `apt-get install libcurl[3|4]-[gnutls|openssl|...]-dev`
- [json-c](http://json-c.github.io/json-c/): `apt-get install libjson-c-dev`
- [modpbase64](https://github.com/client9/stringencoders): `apt-get install libmodpbase64-dev`
- [jpeg](https://github.com/Windower/libjpeg): `apt-get install libjpeg-dev`
- [yajl](https://lloyd.github.io/yajl/): `apt-get install libyajl-dev`
- [SDL2_image](https://www.libsdl.org/projects/SDL_image/): `apt-get install libsdl2-image-dev`
- pthread: already included in libc

## Usage

### Compile
You need CMake, version 3.0 at least. On Debian jessie, you would just do:
```
apt-get install cmake
```
Make sure you have all the dependencies listed above installed. You should have the development version, as pointed out. Then, it is advisable to run CMake in a separate folder to avoid polluting the source tree with CMake's stuff.
```
mkdir build && cd build
cmake -G "Unix Makefiles" ..
```
Verify that a `Makefile` has been created, and then just run:
```
make
```
It should create two executables, namely `appbase-cctv-client` and `appbase-cctv-daemno`, and a shared library, `libappbase-common.so`. This library exports common functions used by both executables.

### Run
So the architecture is fairly simple. There are two pieces, a daemon and a client. Choosing appropriate names was tricky, since the daemon is also a client, strictly speaking.
The daemon takes pictures from the camera in raw YUYV (also known as YUY2) format, converts them to JPEG if requested, and uploads them to Appbase. The client constantly streams them down and displays them.
For this whole system to run, you should have a valid Appbase account beforehand, and have created an application.

You would first run the client:
```
Usage: ./appbase-cctv-client [OPTIONS] <app name> <username> <password>
Options:
    -d    Display debug messages
```
It takes as arguments, your Appbase application name, username and password. So if your app is "myapp", your username is "foo", and your password is "bar", you would run:
```
./appbase-cctv-client -j myapp foo bar
```
You should see a window appearing. As you could see, `-j` says the images were converted to JPEG and thus have to be decoded. This is kind of mandatory, since in my tests, raw images were >100 KB in size, and were rejected by Appbase backend. JPEG conversion greatly reduces them in size.

Now you would run the daemon in a new console, which takes similar arguments. App name, username and password should be the same.
```
Usage: ./appbase-cctv-daemon [OPTIONS] <app name> <username> <password>
Options:
    -w secs        Sleep this amount of seconds between shots
    -d             Display debug messages
    -s             Take one single shot and exit
```
Thus:
```
./appbase-cctv-daemon -jw2 myapp foo bar
```
And you should see the client's window update every 2 seconds.

## Acknowledgements
The author would like to acknowledge the following projects were of great significance during the development of appbase-cctv, and proudly points the reader to them were they interested in learning more about the mechanisms leveraged by the project:
- [uvccapture](https://github.com/csete/uvccapture), for providing a valuable reference on how to interface with UVC cameras via ioctls on Linux.
- [UVC wiki on Ubuntu](https://help.ubuntu.com/community/UVC), for providing documentation about UVC in the first place.
- [LinuxTV](https://linuxtv.org/). The project that officially develops and maintains the UVC drivers in the kernel (and many more cool stuff ;D).
- [Appbase](https://appbase.io/) and its [developer program](https://github.com/appbaseio/recipes/wiki/appbase.io-makers%27-program), for letting me hack this out (and for the stipend too ;D).
