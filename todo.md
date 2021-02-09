### Highest priority:
- Networking
  - It's kinda there, but buggy. Can play 2-player Anarchy games reliably on LAN atm. 
  - Netcode needs to be made portable, currently it sends structs raw through packets, making it sensitive to alignment and platform traits. 
  - The following details need to be tested:
    - Internet play
	- Robo-Anarchy and Co-op game modes.
	- Games with more than 2 players.

- User directory based configuration files. Currently assumes data is in working dir. Won't work on Windows if in program files, or Linux if in the proper locations, and so on...
  - This is critical for Linux to allow installing into the bin dir. 
  - Current plan is as follows:
    - Search the working directory for game data. If found here, the working directory will remain the root of the filesystem.
	- Search some user directory next. (~/.chocolatedescent/descent1 or descent2?) If found here, the user directory will be the root of the filesystem for saves, missions, and so on.
  
- Integrate editor code into Descent 2.
  - Parallax never released the Descent 2 editor source, annoyingly. Need to adapt Descent 1's code.
    - Maybe they'd release it if someone asked? I dunno. 
  - Also need to clean up ui/file.cpp some for potential safety reasons. 
  
- Warningless compile.
  - Going to have to without question invoke my ドＭ side for this one...
  - It's actually not too bad on GCC atm, which is just an indication the warning level isn't high enough.
  - Find some way to tame [MSVC's aggressive and sometimes sketchy static analysis.](https://i.imgur.com/nPfdQHt.png)
    - But pay attention to cases where buffer overruns are actually possible. There's plenty of 'em.
	- Also double check the use of MSVC deprecated stdlib functions. Some issues have been fixed, but many may remain.
	  - Would it be worth using std::string in some contexts?
  
### Lower priority:
- Support for Shareware and OEM builds
  - I've started an effort to reverse-engineer the demo versions of Descent to emulate their behaviors accurately. There are some tricky problems here, like Descent 2 will need to be able to switch between the Descent 1 and 2 texture mappers at runtime for accuracy. 
  
- Poly acceleration emulation
  - Ideally emulating the 3DFX Voodoo and the S3 Virge, is Rendition Verite support considered important? Will require a 15 bit framebuffer that somehow doesn't drag performance to a crawl.
  
- New audio mixer
  - The OpenAL sound code is functional but not very accurate. HMI SOS is known enough that information on its functionality may be available, need to investigate this and write a more accurate software mixer.