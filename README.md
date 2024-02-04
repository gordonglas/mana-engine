# Mana Engine

Simple hobby game engine for Windows 10 and up

This is a hobby project I'm slowly working on and there's still a lot of work needed for this engine to be usable, but if someone can learn from it, that's great. Audio and Input engines are "pretty much" done, but things will be refactored or added as I go. There are some big things still missing, such as graphics (next on my TODO list), scripting, and more. Currently supports Windows 10+ x64 only, but I've been coding in preparation to use it cross-platform.

## Basic architecture:

* `ManaEngine` folder is a static lib that contains the the engine code.
* `ManaGame` folder contains the sample game code. Depends on `ManaEngine`.

The main thread handles the Windows message loop and sends messages to the game loop thread.  
There's a separate thread to handle streaming audio.

## How to build:

* Install [Visual Studio 2022+](https://visualstudio.microsoft.com/vs/) with `Desktop developement with C++` and `Game development with C++` workloads.
* Open `ManaGame/src/msvc/ManaGame/ManaGame.sln` and build the Debug x64 configuration in Visual Studio.
* Install [python3](https://www.python.org/downloads/) so you can run a one-time prepare of the game folder:  
`python ManaGame/scripts/prepare_game_win.py`

## Sample game controls

The sample game currently has controls for testing a looping music file and playing a static sound FX file.  
Run the game at `ManaGame/game/ManaGame.exe`
```
Up arrow - start/resume music
Down arrow - stop music
Left arrow - pause music
Right arrow - resume music
j - play jump sound FX (can play three buffers simultaneously)
```

## Additional engine info:

* Uses XAudio2 (v2.9):
    * It comes pre-installed on Windows 10. The redistributable is only needed for older versions of Windows.
      ManaEngine targets Windows 10+, but it appears that the redist (nuget package) gets updated somewhat often,
      and it's easier to manage with the nuget package.
      See: https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-versions
    * For how to compile with XAudio 2,
      see: https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-redistributable#compiling-your-app
    * Nuget package: https://www.nuget.org/packages/Microsoft.XAudio2.Redist/
      The Nuget package is included in both `ManaEngine` and `ManaGame` projects.
      Note that the Nuget package has `.targets` files that handle linking to the XAudio lib file,
      but only for Release and Debug configurations! So Profile configuration probably won't work with it.
      Be sure to check for updates to the nuget package from time to time and test it.
    * Known issues: Not all calls are compatible with Xbox One. Not compatible with UWP apps.
