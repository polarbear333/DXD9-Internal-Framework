# DirectX Game Modding Framework with ImGui implementation (x86)

Internal modding framework by hooking DirectX9 EndScene function with ImGui implementation, created solely for learning, experimentation purposes.
Ideal for game modders and hackers who want to experiment with D3D9-based games. basic ESP implementation and a framework for DLL injection. 

# Features
* A real-time overlay of enemies (ESP) rendered to screen using WorldToScreen transformation
* Utilizes pattern scanning to locate and modify game addresses
* Hooking game functions to enables game logic modifications (ex. infinite grenade)
* Incorporates ImGui for a user-friendly interface to control game modification options

# Setup
* DirectX SDK: Install the DirectX SDK. You may need to download and install older versions of the DirectX SDK depending on your project's dependencies, include the SDK in PATH enviornment or Visual Studio directory. 
* Visual Studio: Install a compatible version of Visual Studio (2019 or later) with the "Desktop development with C++" workload.
*Visual C++ Redistributable (x86): Download and install the latest version of the Visual C++ Redistributable for Visual Studio 2015-2022 (x86) from the official Microsoft website.
* ImGui (optional): Include the ImGui source code for different build (x64) and integrate them into your project.

# Example Menu:
[![Screenshot-2025-01-11-012256.png](https://i.postimg.cc/G3WpKHWD/Screenshot-2025-01-11-012256.png)](https://postimg.cc/XXg3777N)
