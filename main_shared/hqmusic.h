/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

//Basic HQ music functions

//Plays an OGG file
bool PlayHQSong(const char* filename, bool loop);

//Stops playback of an OGG file
void StopHQSong();
