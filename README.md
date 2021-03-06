# Valkka - OpenSource Video Surveillance and Management for Linux

## For the VERY impatient

Looking for an OpenSource video surveillance program with object detection?

Just go [here](https://elsampsa.github.io/valkka-live/).

## Synopsis

The goal of this project is to provide a library for creating open source video surveillance, management and analysis systems (VMAs) in Linux environment.  The idea is create VMA systems with graphical user interfaces (GUIs) using the combination of python3 and Qt (i.e. PyQt).

## For the impatient

For a demo program that uses libValkka, see [Valkka Live.](https://elsampsa.github.io/valkka-live/)

Installation instructions, demo programs and API tutorial are available [here](https://elsampsa.github.io/valkka-examples/) (you should read that first)

If you just want to use the API, no need to go further.

If you are interested in compiling Valkka yourself or even help us with the core development, keep on reading.

## Why this library?

Most of the people nowadays have a concentration span of milliseconds (because of mobile devices).  Thank you for keep on reading!  :)

Valkka is *not* your browser-based-javacsript-node.js-cloud toy.  We're writing a library for building large ip camera systems in LAN (or virtual-LAN / VPN) environments, capable of doing simultaneously massive live video streaming, surveillance, recording and machine vision.

Lets take a look at a typical video management system architecture problem:
- Stream H264 video from an IP camera using the RTSP protocol
- Branch that stream, and direct it to (1) filesystem and (2) a decoder
- From the decoder, branch the decoded YUV bitmap to (3-5):
  - (3) Analyzer process, using python OpenCV, that inspects the video once per second for car license plates
  - (4) A Fullscreen X-window on screen 1
  - (5) To a smaller X-window on screen 2
- The media stream should be decoded once and only once
- Graphical interface should be based on a solid GUI desktop framework (say, Qt or GTK)

You might try to tackle this with some available stock media player libraries, but I'll promise, you wont get far.

Consider further that in a typical VMA system you may have up to 60+ ip cameras plugged into the same server.  Servers should also work as a proxies, re-streaming the ip cameras to other servers.

Using Valkka, you can instantiate threads, and define how media streams are branched and pipelined between those threads.  The underlying threads and mutex-protected queues are hidden from the developer that controls everything using a python3 API.  The process topology of the example case would look like this:


    [LiveThread]->|                        +-----> [AnalyzerProcess]
                  |                        | (branch 1)
                  +--> [DecoderThread] --->| 
                  |                        | (branch 2)  
                  +--> Filesystem          +------> [OpenGLThread] -- > X window system
             
Some key features of the Valkka library are:
- Python3 API: create process topologies from Python3.
- Develop sleek graphical interfaces fast with PyQt.
- The library itself runs purely in C++.  No python Global Interpreter Lock (GIL) problems here.
- Connections to streaming devices (IP cameras, SDP files) are done using the Live555 media streaming library
- Decoding is done with the FFMpeg library
- Asynchronous texture uploading to GPU using OpenGL.
- Bitmap interpolations with the OpenGL shader language (i.e. GPU does some of the effort)
- The programming architecture makes it possible to implement (some more core development needed):
  - Composite "video in video" (think skype) images
  - Arbitrary geometry transformations : think of fisheye spheres, etc.
  - And much more .. !
- Two-level API.  Level 1 is simply swig-wrapped cpp.  Level 2 is higher level and makes development even easier.
- For more technical details, see [documentation](https://elsampsa.github.io/valkka-core/).  If you are just using the python3 API, you should read at least the "Library Architecture" section.


## Versions and Features

We're currently at alpha

### Latest version is 0.17.0
- Timestamps now taken from decoder!  This means that "main" and "high" H264 profiles with B-frames work.  This should also eliminate some "stuttering" effects seen sometimes in live video.
- "Exotic" bitmaps (YUV422 and other) are now transformed to YUV420, so, for example profiles such as "high422" work (however, this is inefficient, so users should prefer YUV420P streams)

### Older versions

0.16.0
- Yet another memleak at the shmem server side fixed
- Some issues with the python shmem server side fixed

0.15.0
- A nasty memory overflow in the shared memory server / client part fixed
- Added eventfd to the shared mem server / client API.  Now several streams can be multiplexed with select at client side
- Sharing streams between python processes only implemented
- Forgot to call sem_unlink for shared mem semaphores, so they started to cumulate at /dev/shm.  That's now fixed

0.14.1
- Minor changes to valkkafs

0.14.0
- Muxing part progressing (but not yet functional)
- python API 2 level updates

0.13.2 Version
- Extracting SPS & PPS packets from RTSP negotiation was disabled..!
- Now it's on, so cameras that don't send them explicitly (like Axis) should work

0.13.1 Version
- Matroska export from ValkkaFS, etc.
- Lightweight OnVif client

0.12.0 Version
- Shared memory frame transport now includes more metadata about the frames: slot, timestamp, etc.  Now it also works with python multithreading.
- Numpy was included to valkkafs in an incorrect way, this might have resulted in mysterious segfaults.  Fixed that.
- At valkka-examples, fixed the multiprocessing/analyzer example (fork first, then spawn threads)

0.11.0 Version
- Bug fixes at the live555 bridge by Petri
- ValkkaFS progressing
- Currently Ubuntu PPA build fails for i386 & armhf.  This has something to do with the ```off_t``` datatype ?

0.10.0 Version
- Nasty segmentation fault in OpenGL part fixed: called glDeleteBuffers instread of glDeleteVertexArrays for a VAO !
- ValkkaFS progressing

0.9.0 Version
- H264 USB Cameras work

For more, see [CHANGELOG](CHANGELOG.md)

### Long term goals
- Interserver communication and stream proxying
- ValkkaFS filesystem, saving and searching video stream
- A separate python3 Onvif module

### Very long term goals
- A complete video management & analysis system

## Installing

Binary packages and their Python3 bindings are provided for latest Ubuntu distributions.  Subscribe to our repository with: 

    sudo apt-add-repository ppa:sampsa-riikonen/valkka
    
and then do:
    
    sudo apt-get update
    sudo apt-get install valkka

## Compile yourself

### Dependencies

You need (at least):

    sudo apt-get install build-essential libc6-dev yasm cmake pkg-config swig libglew-dev mesa-common-dev libstdc++-5-dev python3-dev python3-numpy libasound2-dev valgrind
    
### Compile

This just got a lot easier: the same CMake file is used to compile the library, generate python wrappings and to compile the wrappings (no more python setup scripts)

Valkka uses numerical python (numpy) C API and needs the numpy C headers at the build process.  Be aware of the numpy version and header files being used in your setup.  You can check this with:

    ./pythoncheck.bash
    
We recommend that you use a "globally" installed numpy (from the debian *python3-numpy* package) instead of a "locally" installed one (installed with pip3 install).  When using your compiled Valkka distribution, the numpy version you're loading at runtime must match the version that was used at the build time.

Next, download live555 and ffmpeg

    cd ext
    ./download_live.bash
    ./download_ffmpeg.bash
    cd ..

Next, proceed in building live555, ffmpeg and valkka 
    
    make -f debian/rules clean
    make -f debian/rules build
    
Finally, create a debian package with

    make -f debian/rules package
    
You can install the package to your system with

    cd build_dir
    dpkg -i Valkka-*.deb
    sudo apt-get -fy install
    
### Development environment
    
If you need more fine-grained control over the build process, create a separate build directory and copy the contents of the directory *tools/build* there.  Read and edit *run_cmake.bash* and *README_BUILD*.  Now you can toggle various debug/verbosity switches, define custom location for live555 and ffmpeg, etc.  After creating the custom build, you should run

    source test_env.bash

in your custom build directory.  You still need to inform the python interpreter about the location of the bindings.  In the main valkka directory, do:

    cd python
    source test_env.bash
    
And you're all set.  Now you have a terminal that finds both libValkka and the python3 bindings

### Semi-automated testing

After having set up your development environment, made changes to the code and succesfully built Valkka, you should run the testsuite.  Valkka is tested by a series of small executables that are using the library, running under valgrind.  For some of the tests, valgrind can't be used, due to the GPU direct memory access.  For these tests, you should (i) run them without valgrind and see if you get video on-screen or (ii) compile valkka with the VALGRIND_DEBUG switch enabled and only after that, run them with valgrind.

In your build directory, refer to the bash script *run_tests.bash*.  Its self-explanatory.

Before running *run_tests.bash" you should edit and run the *set_test_streams.bash* that sets up your test cameras.


## Resources

1. Discussion threads:

  - [GLX / OpenGL Rendering](https://www.opengl.org/discussion_boards/showthread.php/200394-glxSwapBuffers-and-glxMakeCurrent-when-streaming-to-multiple-X-windowses)

2. Doxygen generated [documentation](https://elsampsa.github.io/valkka-core/)
3. The examples [repository](https://github.com/elsampsa/valkka-examples)

## Authors
Sampsa Riikonen (core programming, opengl shader programming, python programming)

Petri Eranko (financing, testing)

Marco Eranko (testing)

Markus Kaukonen (opengl shader programming, testing)

## Acknowledgements

Ross Finlayson

Dark Photon

GClements

## Copyright
(C) 2017, 2018 Valkka Security Ltd. and Sampsa Riikonen

## License
This software is licensed under the GNU Affero General Public License (AGPL) v3 or later.

If you need a different license arrangement, please contact us.
