/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/

#include "platform/posixstub.h"

int _filelength(int fd)
{
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}