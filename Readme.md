# Exercises for the practical session for INFO-H502

The cmake files used from this project are heavily inspired from Glitter https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt   

## Getting Started

Make sure that git is installed on your computer. If not download it at https://git-scm.com/downloads or use `sudo apt-get install git` (in terminal for Linux)

Clone the project (or fork it) in a **terminal** :
```
git clone --recursive https://gitlab.com/lisa-vr-course/info-h502_202425.git 
```
If you forgot to add `--recusive` flag (or want to update the submodules) use this:
```
git submodule update --init --recursive
```

If you didn't follow the above instructions, download the code in the form of a zip from the gitlab, you will have to use git to add the submodules yourself.  
Delete the glfw, stb and glm folders that are empty (not the glad one !)  
In a terminal (there no escape to the terminal) use cd and ls/dir to place yourself in the folder containing the code:
```
git init

git submodule add https://github.com/glfw/glfw 3rdParty/glfw
git submodule add https://github.com/g-truc/glm 3rdParty/glm
git submodule add https://github.com/nothings/stb 3rdParty/stb
```

Note for Linux users:  
If you have a fresh installation of Linux you may also need to install `sudo apt-get install build-essential` as well as drivers for amd, nvidia or Mesa

If you have an error with cmake saying that it does not find opengl, you can install the `mesa-common-dev`
If you have other errrors speaking about `libxrandr`, `libxinerama`, `libxcursor` and `libxi` you may also need to install them.
Here is the list of command you may need to do with Linux (with X11)`, just pick the ones you need:

```
sudo apt-get install mesa-common-dev
sudo apt-get install libxrandr-dev
sudo apt-get install libxinerama-dev
sudo apt-get install libxcursor-dev
sudo apt-get install libxi-dev
```


## Setup your development environment and build your code

You have plenty of option to setup your development environment and build your code.
For conveniance, we recommend you an IDE that directly support cmake. 
Here is some of them:
### Setup an IDE that support CMake build (recommanded option)
#### Visual Studio (Windows only)  
- Install Visual Studio (Community) if not already done : https://visualstudio.microsoft.com/downloads/ 
- Install the component "Desktop development with C++" during the installation process and be sure that "C++ Cmake tools for windows" is installed.
- Open Visual Studio and select "Open a local folder" then select the folder that you just clone.

#### Visual studio code (Cross-platform)
- Install Visual Studio Code : https://code.visualstudio.com/download
- Install CMake : https://cmake.org/download/
- Install the VSC extension for C++ and CMake tools.
- In VSC, select your C++ compiler, mode (Debug, Release, ...) then run ```Cmake:Configure```
- Build your code then run it.
- You can check this tutorial : https://code.visualstudio.com/docs/cpp/cmake-linux
It's fairly similar for MacOs.


#### Qt creator

- Install cmake if not already done: https://cmake.org/download/ or `sudo apt-get install cmake` (Linux)
- Install Qt creator (open-source): https://www.qt.io/download-open-source 
- Open Qt Creator and select `Open project`
- Chose the default configuration and press configure project


#### CLion

- Install CLion.
- Check out the tutorial for cmake with CLion : https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html
