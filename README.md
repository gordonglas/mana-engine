# ManaEngine - 2d in 3d game engine for Windows 7 and up

## TODO:
* audio engine
    * DONE-Call CreateSourceVoice "SimultaniousSounds" times within Load and store in vector, and Play can loop through them.
    * DONE-IsPlaying, IsPaused
    * DONE-Stop, StopAll
    * DONE-Pause, PauseAll, Resume, ResumeAll
    * DONE-AudioCategory = {SoundFx, Music, Voice} - allows us to control a group of related sounds with volume or mute/unmute.
    * DONE-AudioLoadType = {Static, Streaming}
    * DONE-AudioFormat = {Wav, Ogg Mod}
    * DONE-SetVolume (of all voices of a specific file), SetVolume (by AudioCategory), SetMasterVolume (sets master voice volume - affects all files)
    * DONE-SetPan: https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--pan-a-sound
    * NO-MuteAll (save current volumes), UnmuteAll (restore current volumes)
        * While global IsMuted, newly created sounds should be created with the volume 0, while their "last volume" should be set to 1.0f.
    * DONE-IsPlaying, IsPaused
    * DONE-allow to play looped (and specify loop sample range)
    * NO-Stop a looping sound (could call Voice::ExitLoop, but most use-cases can probably just call Stop)
    * DONE-function to unload file
    * DONE-function to unload XAudio2
    * LATER-Pitch functions? https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-volume-and-pitch-control#pitch-control (Voice->SetFrequencyRatio)
    * NO-Seek function? https://stackoverflow.com/questions/16649023/how-to-seek-to-a-position-in-milliseconds-using-xaudio2
        * May never need this, since we support a loop region in Play.
    * stream libopenmpt through xaudio2
    * stream ogg vorbis through xaudio2 (see notes after code in Game Coding Complete)
    * some way to handle copying the xaudio dll to the game folder. Post build step, or one-time setup script?
    * Test running on Windows 7 laptop to make sure all used XAaudio2.9 api calls are supported.
    * LATER-static load ogg vorbis through xaudio2 (see code in Game Coding Complete)
    * LATER-GetLengthMillis (see code in Game Coding Complete - or can probably calculate it ourselves.)

* merge stuff from ManaEngine_OLD and ManaGame_OLD
    * ManaEngine_OLD has `ManaServerConsole.cpp` (winsock)
* Write golang app to mimic `git lfs` sha256 and store map of filename to hash.
    * where should this run?
    * add to new git repo (with git lfs rules)


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
  -Move `framework.h` to the `inc` folder.
  -Move `ManaEngine.cpp` to the `src` folder.

-In solution explorer:
  Consolidate Headers and Source filters into new `src` filter.
  Update the newly moved files to be under the new `src` filter.

-In project properties:
  -Fix intelisense issue for when `#include`ing the `pch.h` header file:
    See: https://stackoverflow.com/a/12770937/341942

  -Select `All configurations` and `All Platforms`, then set the following:
    -General -> Output Directory: `$(ProjectDir)..\..\..\lib\$(PlatformName)$(Configuration)\`
    -General -> Intermediate Directory: `$(ProjectDir)..\..\..\temp\$(ProjectName)$(PlatformName)$(Configuration)\`
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


## Additional Engine info:

* Uses XAudio2 (v2.9 for Windows 7 SP1 support):
    * https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-redistributable#installing-the-nuget-package
    * Nuget package: https://www.nuget.org/packages/Microsoft.XAudio2.Redist/
    * Manual package install instructions:
        * Rename nuget package to `.zip` extension and extract files under `ManaEngine\third-party\xaudio2`.
        * Add include folder to include search paths, above the Windows SDK, so it uses this version instead.
        * Add Release lib folder for ALL configurations. The `.targets` file in the nuget package has a comment that says to do this, since the Debug version hasn't been well tested.
    * Known issues: Not all calls are compatible with Xbox One. Not compatible with UWP apps.
    * XAudio2 v2.9 dll doesn't come with Windows 7, so we will need to check/deploy it in our installer. (Or maybe we just include it in same folder as exe? Check docs)


## How to create a new Game project:

-Create folders under GameDev root:
  ManaGame  -Replace with the name of your game.
    assets  -Raw art/music/sound files. Probably only needed for game projects.
      art
        user-manual
      music
      sound
    bin     -built binaries
    docs    -Internal docs/notes.
    game    -Everything needed to run the game. Only needed for Game projects.
    src     -C++ files
      msvc  -Visual Studio specific files. Solution/project files, etc.
    temp    -Temp compiler files.
    test    -Utilities for testing.

-Create new Visual Studio 2019 project:
  -Empty Project, or Windows Desktop Application (c++)
    Project Name: ManaGame
    Location: D:\Users\gglas\Devel\Gamedev\ManaGame\src\msvc
    Check checkbox "Place solution and project in the same directory"
    Click Next.

-In File Explorer:
  Move `.h` and `.cpp` files to the `src` folder.

-In solution explorer:
  Consolidate Headers and Source filters into new "src" filter.
  Remove the .h/cpp files that no longer exist and add the ones under src folder.

-In project properties:
  -Select `All configurations` and `All Platforms`, then set the following:
    -General -> Output Directory: `$(ProjectDir)..\..\..\lib\$(PlatformName)$(Configuration)\`
    -General -> Intermediate Directory: `$(ProjectDir)..\..\..\temp\$(ProjectName)$(PlatformName)$(Configuration)\`
    -Debugging -> Working Directory: `$(ProjectDir)..\..\..\Game\`
    -VC++ Directories -> Include Directories -> Edit -> New Line at top: `$(ProjectDir)..\..\..\..\ManaEngine\inc`
    -VC++ Directories -> Library Directories -> Edit -> New Line at top: `$(ProjectDir)..\..\..\..\ManaEngine\lib\$(PlatformName)$(Configuration)`
    -C/C++ -> General -> Warning: `Level 4 (/W4)`
    -Linker -> General -> Additional Library Directories: `$(ProjectDir)..\..\..\..\ManaEngine\lib\$(PlatformName)$(Configuration);%(AdditionalLibraryDirectories)`
    -Linker -> Input -> Additional Dependencies: `ManaEngine.lib;%(AdditionalDependencies)`

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
* Follow the instructions for the Game Coding Complete README, except checkout code from the above fork instead of svn, and use the third party code downloaded from above.
    * README: https://github.com/georgerbr/gamecode4/tree/vs2017

Source: https://www.mcshaffry.com/GameCode/index.php/Thread/2263-Compilers-2017/?postID=13724#post13724  
