============================================================
  HypeClip AI - How to install  (plain-English version)
============================================================

WHAT YOU HAVE RIGHT NOW
-----------------------
- HypeClipAI.zip ............ the plugin's source code (the "recipe")
- Install-HypeClip-AI.bat ... a double-click installer (does the copying for you)
- INSTALL-README.txt ........ this file

THE ONE THING TO UNDERSTAND
---------------------------
A plugin is a program. Right now you have the *instructions* for the program,
not the finished program. The finished program is a single file called:

        hypeclip-ai.dll

That file has to be "built" (compiled) once on a Windows computer that has the
developer tools. After that, installing is just a double-click forever.

You cannot install the plugin until that .dll file exists. There is no way
around this - it's true for every OBS plugin.

HOW TO GET THE hypeclip-ai.dll FILE  (easiest options first)
------------------------------------------------------------

OPTION A - Have someone build it for you (recommended if you're not technical)
   Send the HypeClipAI.zip to anyone comfortable with C++/OBS development
   (a developer friend, or a freelancer on Fiverr/Upwork - search
   "compile OBS plugin"). For someone with the setup it's about 15 minutes.
   Ask them to send you back:
        - hypeclip-ai.dll
        - the "data" folder
   Tell them: "It's a standard obs-plugintemplate CMake project, Windows x64,
   Qt6. Build Release."

OPTION B - Build it in the cloud for free (no tools to install on your PC)
   The code is set up to compile automatically on GitHub using GitHub Actions.
   You upload the code to a free GitHub account, and GitHub builds the .dll for
   you and gives you a download link. If you'd like this, just ask me to
   "set up the GitHub auto-build" and I'll add the workflow file and write you
   click-by-click steps.

OPTION C - Build it yourself
   Follow the OBS plugin template guide:
   https://github.com/obsproject/obs-plugintemplate
   (This is the most involved option - only if you enjoy tinkering.)

ONCE YOU HAVE hypeclip-ai.dll
-----------------------------
1. Put "hypeclip-ai.dll" and the "data" folder in the SAME folder as
   Install-HypeClip-AI.bat.
2. Make sure OBS Studio is CLOSED.
3. Double-click  Install-HypeClip-AI.bat.
4. Open OBS, then click:  Docks  >  HypeClip AI.
5. Done. It captures highlights automatically while you play.

(The installer copies into your personal OBS folder, so it never asks for an
admin password and can't harm anything. To uninstall, just delete the folder:
 %APPDATA%\obs-studio\plugins\hypeclip-ai )

Need help? Ask me to set up the free cloud build (Option B) and I'll walk you
through it step by step.
