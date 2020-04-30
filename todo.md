### Large scale changes:
- Integrate editor code.
  - Will require cleaning up the UI code some, in particular getting portable directory iteration code working in file.cpp

- Tests
  - For all the arcane typing changes done in the math code for cross compiler support...

- Warningless compile.
  - Going to have to without question invoke my ドＭ side for this one...
  - It's actually not too bad on GCC atm, which is just an indication the warning level isn't high enough.
    - Except the Descent 2 code, which needs cases of `const char*s` being passed to `char*s` fixed. 
  - fuck MSVC's deprecation warnings, but pay attention to cases where buffer overruns are actually possible. There's plenty of 'em.
  - Find some way to tame [MSVC's aggressive and sometimes sketchy static analysis.](https://i.imgur.com/nPfdQHt.png)

- User directory based configuration files. Currently assumes data is in working dir. Won't work on Windows if in program files, or Linux if in the proper locations, and so on...
  - Pretty much vital for a proper Linux bundle. 
  
- New audio mixer
  - The OpenAL sound code is functional but not very accurate. HMI SOS is known enough that information on its functionality may be available, need to investigate this and write a more accurate software mixer.
  
- Poly acceleration emulation
  - Ideally emulating the 3DFX Voodoo and the S3 Virge, is Rendition Verite support considered important? Will require a 15 bit framebuffer that somehow doesn't drag performance to a crawl.
  
### Lower priority changes:
- Support for Shareware and OEM builds
  - Descent 1 shareware shouldn't be bad, Descent 2 shareware will require a lot of tweaks. Descent 2 shareware is also hampered by my desire for reasonably accurate emulation - which includes features like weaker headlights, long positional sounds being synced, guidebots firing more frequently in their cages, Omega being entirely different, Earthshakers not having children, faster and brighter briefing text, bosses using the Descent 1 boss logic, a different HAM and map format, among many others... and yes half of those features aren't available without modding...

###### Changes that I don't know the scale of tbh...:
 - MIDI music in the native Win32 audio backend. 
