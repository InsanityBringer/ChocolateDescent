Chocolate Descent, a barebones software-rendered focused port of the Descent game.

Building:
This is very crude ATM, and will hopefully be seriously improved later down the line.

A solution for Visual Studio 2019 is included. At the moment it is not set to be built release
To build it, SDL 2 and OpenAL Soft are required. Projects gr and Main must be pointed at SDL's includes
Project bios needs to be pointed at OpenAL Soft's includes, and Main needs to be pointed at the library
directories for both.

Chocolate Descent needs game data to run. At the moment, it will only work with the HOG and PIG files
from Descent's PC release, versions 1.4 or 1.5. 

License:
All the original code is under the terms of Parallax's Source License. New contributions
are available under the terms of the MIT License, as described in COPYING.TXT