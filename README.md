# MultiFive source code:

That is source code of multifive (old fivem).
That is mini instruction how to compile it on Windows:

## What do we need?

- [visual studio 2015, you may use community edition](https://www.visualstudio.com/ru-ru/downloads/download-visual-studio-vs.aspx)
- [boost 1.57](http://www.boost.org/doc/libs/1_57_0/)
- [depot tools](https://www.chromium.org/developers/how-tos/install-depot-tools)

## How to start code in VS?

First of all, you must to install all of libs. Than, you must create your folder with any name and make command in CMD:
```
cd C:"your path"
```
Next step is to make next commands in CMD:
```
gclient config --unmanaged https://gitlab.com/multifive/multifive-core.git
gclient sync
cd multicore
git checkout portability-five
gclient sync
```
> **_DO NOT CLOSE CMD!_**
After all of that steps you must download premake5 from [this site](https://premake.github.io/) than use the command:
```
premake5.exe vs2015 --game=five
```
your solution will be in multicore/build.
**Good Luck!**

## How to start the game?

- download [fivem-cache](https://gitlab.com/multifive/fivem-cache/) in any folder and select all of fivem-cache/caches/fivem place in your multifive folder
- start fivem.exe