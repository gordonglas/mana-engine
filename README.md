# ManaEngine - 2d in 3d game engine for Windows 7 and up

## How to create a new Static Library project:

-Create folders under GameDev root:
  ManaEngine
    docs    -Internal docs/notes.
    lib     -Static lib output. Only needed for static lib projects.
    inc     -Header files for static lib. Only needed for static lib projects.
    src     -C++ files
      msvc  -Visual Studio specific files. Solution/project files, etc.
    temp    -Temp compiler files.
    test    -Utilities for testing.

-Create new Visual Studio 2019 project:
  -static library (c++)
    Project Name: ManaEngine
    Location: D:\Users\gglas\Devel\Gamedev\ManaEngine\src\msvc
    Check checkbox "Place solution and project in the same directory"
    Click Next.

-In File Explorer:
  -move `framework.h` to the `inc` folder.
  -Move `ManaEngine.cpp` to the `src` folder.

-In solution explorer:
  Consolidate Headers and Source filters into new `src` filter.
  Update the newly moved files to be under the new `src` filter.

-In project properties:
  -Fix intelisense issue for when `#include`ing the `pch.h` header file:
    See: https://stackoverflow.com/a/12770937/341942

  -Select `All configurations` and `All Platforms`, then set the following:
    -General -> Output Directory: `$(SolutionDir)..\..\..\lib\$(PlatformName)$(Configuration)\`
    -General -> Intermediate Directory: `$(SolutionDir)..\..\..\temp\$(ProjectName)$(PlatformName)$(Configuration)\`
    -VC++ Directories -> Edit -> New Line: `$(ProjectDir)`
    -VC++ Directories -> Edit -> New Line: `$(ProjectDir)..\..\..\inc\` (and move it just under `$(ProjectDir)`)
    -C/C++ -> General -> Warning: `Level 4 (/W4)`

-Create a "Profile" build configuration:
  -Build menu -> Configurations -> Active solution configuration -> New
    -Name: Profile
    -Copy settings from: Debug
    -Click OK

-In project properties:
  -Select Configuration `Profile` and Platform `Win32`, then set the following:
    -C/C++ -> Preprocessor -> Preprocessor Definitions: Replace `_DEBUG` with `NDEBUG` (it should look same as `Release`)
  -Select Configuration `Profile` and Platform `x64`, then set the following:
    -C/C++ -> Preprocessor -> Preprocessor Definitions: Replace `_DEBUG` with `NDEBUG` (it should look same as `Release`)

-Build all configurations.

TODO: merge stuff from ManaEngine_OLD and ManaGame_OLD
TODO: add to new git repo


## How to create a new Game project:

-Create folders under GameDev root:
  ManaEngine
    assets  -Raw art/music/sound files. Probably only needed for game projects.
      art
        user-manual
      music
      sound
    docs    -Internal docs/notes.
    game    -Everything needed to run the game. Only needed for Game projects.
    lib     -Static lib output. Only needed for static lib projects.
    inc     -Header files for static lib. Only needed for static lib projects.
    src     -C++ files
      msvc  -Visual Studio specific files. Solution/project files, etc.
    temp    -Temp compiler files.
    test    -Utilities for testing.

-Create new Visual Studio 2019 project:
  -static library (c++)
    Project Name: ManaEngine
    Location: D:\Users\gglas\Devel\Gamedev\ManaEngine\src\msvc
    Check checkbox "Place solution and project in the same directory"
    Click Next.

-In File Explorer:
  If this is a static library, move framework.h to the inc folder, else the src folder.
  Move ManaEngine.cpp to the src folder.

-In solution explorer:
  Consolidate Headers and Source filters into new "src" filter.
  Remove the .h/cpp files that no longer exist and add the ones under src folder.

-In project properties:
  -Fix intelisense issue for when `#include`ing the `pch.h` header file:
    See: https://stackoverflow.com/a/12770937/341942

  -Select `All configurations` and `All Platforms`, then set the following:
    -General -> Output Directory: `$(SolutionDir)..\..\..\lib\$(PlatformName)$(Configuration)\`
    -General -> Intermediate Directory: `$(SolutionDir)..\..\..\temp\$(ProjectName)$(PlatformName)$(Configuration)\`
    -VC++ Directories -> Edit -> New Line: `$(ProjectDir)`
    -if this is a static library:
      -VC++ Directories -> Edit -> New Line: `$(ProjectDir)..\..\..\inc\` (and move it just under `$(ProjectDir)`)
    -C/C++ -> General -> Warning: `Level 4 (/W4)`

-Create a "Profile" build configuration:
  -Build menu -> Configurations -> Active solution configuration -> New
    -Name: Profile
    -Copy settings from: Debug
    -Click OK

-In project properties:
  -Select Configuration `Profile` and Platform `Win32`, then set the following:
    -C/C++ -> Preprocessor -> Preprocessor Definitions: Replace `_DEBUG` with `NDEBUG` (it should look same as `Release`)
  -Select Configuration `Profile` and Platform `x64`, then set the following:
    -C/C++ -> Preprocessor -> Preprocessor Definitions: Replace `_DEBUG` with `NDEBUG` (it should look same as `Release`)

-Build all configurations.

TODO: merge stuff from ManaEngine_OLD and ManaGame_OLD
TODO: add to new git repo


## How to build Game Coding Complete 4th edition code for Visual Studio 2017:

* Install Visual Studio 2017.
    * Install the Windows 8.1 SDK
* Install the June 2010 DirectX SDK.
    * if you get Error Code s1023, it means you have a newer VC++ 2010 redistributable installed. To fix:
        * Uninstall both Microsoft Visual C++ 2010 x86 & x64 Redistributables.
        * Install the June 2010 DirectX SDK.
        * Reinstall the latest Microsoft Visual C++ 2010 Redistributables:
            * x86: https://www.microsoft.com/en-us/download/details.aspx?id=8328
            * x64: https://www.microsoft.com/en-us/download/details.aspx?id=13523
        * Source: https://support.microsoft.com/en-us/help/2728613/s1023-error-when-you-install-the-directx-sdk-june-2010
* Grab `gamecode4` code from georgerbr's fork, `vs2017` branch: https://github.com/georgerbr/gamecode4/tree/vs2017
* The updated libraries are available here: https://www.dropbox.com/s/3r16mdr83z7njuu/3rdParty_v4.0-VS2017.zip?dl=0
    * If they are no longer there, you can alternatively grab them from me here: https://drive.google.com/file/d/1T3BmBLguZtgnGJspwAayTETgalhd-aD1/view?usp=sharing
* Follow the instrunctions for the Game Coding Complete README, except checkout code from the above fork instead of svn, and use the third party code downloaded from above.
    * README: https://github.com/georgerbr/gamecode4/tree/vs2017

Source: https://www.mcshaffry.com/GameCode/index.php/Thread/2263-Compilers-2017/?postID=13724#post13724  
