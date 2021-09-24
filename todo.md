### Highest priority:
- User directory based configuration files. Currently assumes data is in working dir. Won't work on Windows if in program files, or Linux if in the proper locations, and so on...
  - Code provided for the macOS implementation, needs testing on other platforms. Windows should also always accept the working directory. 
  - This is critical for Linux to allow installing into the bin dir. 
  
- Warningless compile.
  - Going to have to without question invoke my ドＭ side for this one...
  - It's actually not too bad on GCC atm, which is just an indication the warning level isn't high enough.
  - Find some way to tame [MSVC's aggressive and sometimes sketchy static analysis.](https://i.imgur.com/nPfdQHt.png)
    - But pay attention to cases where buffer overruns are actually possible. There's plenty of 'em.
	- Also double check the use of MSVC deprecated stdlib functions. Some issues have been fixed, but many may remain.
	  - Would it be worth using std::string in some contexts?
  
### Lower priority:
- Support for Shareware and OEM builds
  - Descent 2 Interactive Demo is now supported, alongside a framework to allow later reintegration of the OEM code. Descent 1 will be triciker, since all versions use data files named the same.
  - Descent 1 will identify different versions by looking at the contents of the HOG file and seeing what is and isn't present. This is also needed to detect Descent 2's OEM builds. 
  
- Poly acceleration emulation
  - Ideally emulating the 3DFX Voodoo and the S3 Virge, is Rendition Verite support considered important? Will require a 15 bit framebuffer that somehow doesn't drag performance to a crawl.
  
- New audio mixer
  - The OpenAL sound code is functional but not very accurate. HMI SOS is known enough that information on its functionality may be available, need to investigate this and write a more accurate software mixer.

- Really, I do still wonder if parallax would release the Descent 2 editor source if someone asked, therefore sparing my unholy fusion of Descent 1's editor with Descent 2, or if any further source releases are entirely at the whim of Interplay...
