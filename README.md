# OpenGLAssignment

An OpenGL class assignment.

## How to compile

You will need glm-0.9.7.5, glew-1.10.0-win32 and freeglut-MSVC-2.8.1-1.mp, extract them
and put them in "C:\OpenGLwrappers\" (or anywhere else, but you will need to edit CMakeLists.txt
manually for the compiler to work).

### 如何使用Visual Studio编译运行

*因为本作业是使用CLion完成的，使用Visual Studio的用户如果不想手动修改代码，就按照以下步骤操作以使用Visual Studio编译运行。*

事前准备：git clone 本仓库，安装Python3（所有用户）以及CMake（英文版）  
（以及把glm-0.9.7.5, glew-1.10.0-win32和freeglut-MSVC-2.8.1-1.mp这三个库放在"C:\OpenGLwrappers\"中，如果你有上计算机图形学的实验课那么你大概已经做过了。）

启动CMake，在"Where is source code"中选择本项目的文件夹（即包含CMakeLists.txt的文件夹）
在“Where to build”中指定一个文件夹用于存放编译生成的结果（为了减少工作量，请将此文件夹设定在本项目文件夹中，如/OpenGLAssignment/build）。

接下来，选择"Configure"，生成器选择Visual Studio(版本对应你的Visual Studio版本)，"Optional platform for generator"选择**Win32**（否则编译会失败）。点击"Finish"。

"Configure"完成后，点击"Generate"，等待生成完成后点击"Open Project"，便会在Visual Studio中打开本项目，此时打开“解决方案资源管理器”，右击OpenGLAssignment，选择“设为启动项目”，接下来就可以直接点绿色小三角编译运行了！


### Using CLion

git clone this repository, and you are ready to compile (probably).

### Using Visual Studio

Download and install some version of Microsoft Visual Studio with C++ support,
install Python 3 for all users and install CMake.

Start CMake and select the source code folder (the directory
with CMakeLists.txt). Additionally select a build folder, e.g. create a build
subdirectory in the source code directory. Click "Configure" and select the
Visual Studio generator, **for Optional Platform select Win32**. 
After configuration finishes and the "Generate" reactivates, click
it. When that finishes, click "Open Project". Visual Studio should open. 
You can compile the project by right-clicking the OpenGLAssignMent (not the 
solution) and select "Select as StartUp project". Now you should be able to 
compile by clicking the green, triangular "Run" button.
---

For more information, see [TODOlist](TODOlist.md).
