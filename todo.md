### Large scale changes:
- Integrate editor code.
  - Will require cleaning up the UI code some, in particular getting portable directory iteration code working in file.cpp

- Tests
  - For all the arcane typing changes done in the math code for cross compiler support...

- Warningless compile.
  - Going to have to without question invoke my ドＭ side for this one...
  - It's actually not too bad on GCC atm, which is just an indication the warning level isn't high enough.
    - Except the Descent 2 code, which needs cases of const char*s being passed to char*s fixed. 
  - fuck MSVC's deprecation warnings, but pay attention to cases where buffer overruns are actually possible. There's plenty of 'em.
  - Find some way to tame [MSVC's aggressive and sometimes sketchy static analysis.](https://i.imgur.com/nPfdQHt.png)

- User directory based configuration files. Currently assumes data is in working dir. Won't work on Windows if in program files, or Linux if in the proper locations, and so on...
  - Pretty much vital for a proper Linux bundle. 

###### Changes that I don't know the scale of tbh...:
    MIDI music in the native Win32 audio backend. 
