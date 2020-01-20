tileworld-for-handhelds
=======================

A port of Tile World to 320x240 handheld consoles including GP2X, Wiz, and GCW Zero/Dingux




  Welcome to Tile World

Tile World is an emulation of the game "Chip's Challenge" for the Atari
Lynx, created by Chuck Sommerville, and later ported to MS Windows by
Microsoft (among other ports).

  Important Note

Tile World is an emulation of the "Chip's Challenge" game engines only. It
does not come with the chips.dat file that contains the original level set.
That file, which is copyrighted and cannot be freely distributed, was
originally distributed with the MS version of "Chip's Challenge". If you
have a copy of this version of the game, you can use that file to play the
original games in Tile World. If you do not have a copy of this file,
however, you can still play Tile World with the many freely available level
files created by fans of the original game.

  Installing Tile World under Windows

First of all, you'll want to store the files contained in this archive into
its own separate directory. If you're using the self-extracting executable,
you can create a new directory during the installation. Otherwise, you'll
need to create a new directory beforehand -- something like c:\tworld --
and extract the files in there.

If you have a copy of the chips.dat data file, copy it to the data
subdirectory. This will allow you to play the original levels under Tile
World (for the MS ruleset and the Lynx ruleset both).

If you have other data files that you would like to try out in Tile World,
copy those to the sets directory.

The shell commands to do the above would look something like:

  cd c:\wherever\my\copy\of\chips\challenge\is\at
  copy chips.dat c:\tworld\data
  cd c:\my\collection\of\dat\files
  copy *.dat c:\tworld\sets

That's all that needs to be done to set it up. Run the program as
c:\tworld\tworld, or create a shortcut for it.

  Installing Tile World under Linux

Before proceeding, ensure that you have SDL installed on your machine. (If
you don't have SDL, you can get it by visiting
http://www.libsdl.org/download.html. If you download a precompiled version
-- i.e., an .rpm or .deb file -- note that you will need the development
runtime, as opposed to the binary runtime.)

Installing Tile World involves the usual three-part incantation:

  ./configure

By default, the program is set up so that it will keep its shared files
under /usr/local/share/tworld. If you would prefer the tworld directory to
be somewhere besides /usr/local/share, use the --datadir option to change
it when you run ./configure. Alternately, you can use the
--with-sharedir=DIR option to explicitly specify a completely different
path. (This value can also be changed at runtime, either via the TWORLDDIR
environment variable or via the command line.)

  make

There shouldn't be any serious warnings from the compiler. Use "make
mklynxcc" if you want to also build a copy of mklynxcc (see below).

  make install

Running "make install" as root will do the following:

* Copy the tworld binary to /usr/local/games.
* Copy the tworld.6 manpage to /usr/local/man/man6.
* Create /usr/local/share/tworld if it does not exist. (Or whatever
  directory you specified to ./configure.)
* Copy the external resources (i.e., the bitmaps and wave files) to
  /usr/local/share/tworld/res.
* Create the directories /usr/local/share/tworld/data and
  /usr/local/share/tworld/sets.

The sets directory is where you will generally store the .dat files that
you want to use. However, if you want to make use of a configuration file
with a particular data file, then you will need to store the data file in
the data directory, and the configuration file goes into the sets directory
instead. See the documentation for more information.

  Level Sets

As mentioned above, the original "Chip's Challenge" level set does not come
with Tile World, for reasons of copyright. If you do not already have a
copy of Microsoft's Windows version of "Chip's Challenge", you might still
be able to find a copy. Search the links listed below, under "Resources on
the Internet", for helpful hints on finding the game online.

If and when you do, you can copy the chips.dat file from there into Tile
World's data directory. You will then be able to play the levels of the
original set (both in MS mode and in Lynx mode).

There are also many "user-created" level sets. These are sets of levels
which have been invented by fans of the game. These sets are freely
available for downloading. If you have a .dat file that contains a level
set and you wish to use it, just copy it to Tile World's sets directory.
The next time you start Tile World, the new .dat should appear in the list
of available level sets.

At http://groups.yahoo.com/group/chips_challenge/files/ is a repository of
the available level sets that have been created. The biggest fans of the
game try to provide a copy of every known user-created level set at this
place. I have not included any of these level sets in this distribution, as
the authors continue to add new levels to their sets over time.

Actually, this distribution does contain one small level set. This is
included so that even if you don't have the original level set, you can
still get a brief glimpse of how the game works, and what some of the most
basic challenges are. Also, the same set of levels can be played with both
the MS and Lynx ruleset, so you can see how they differ.

Finally, there is one level set that is special and deserves particular
mention. That is CCLP2.dat, or "Chip's Challenge Level Pack 2". This set
was assembled by the fans, who voted on all of the user-created levels that
existed at the time. The levels that were voted as being the most fun were
then all put together to become CCLP2. It is the closest thing we have to a
sequel for the original game. (But be careful: CCLP2 is much harder than
the original!) You can download a copy of Tile World with this set already
installed. If instead you download it separately, you will want to store it
in your data directory instead of your sets directory. (This is because
Tile World comes with a special configuration file for CCLP2.)

  The Complete Documentation

The full documentation for Tile World is included with the distribution, in
the file tworld.html. There you will find information on how to play the
game, adding new level sets, customizing Tile World, and more.

  Creating New Level Sets

The most widely used program for creating new level sets is ChipEdit. It
comes with excellent documentation, and you should have little trouble
learning how to use it. Some other editors have recently been made
available, such as CCEdit and Chip's Workshop.

Normally, ChipEdit creates levels for the MS ruleset. If you wish to make a
level set for the Lynx ruleset, you have a few options:

* A very simple command-line utility is included with Tile World, called
  mklynxcc. This program will change a normal .dat file to one that will
  use the Lynx ruleset instead of the MS ruleset. Running mklynxcc foo.dat
  will change foo.dat's ruleset from MS to Lynx.
* You can use a configuration file to override the builtin ruleset. This
  method requires creating an extra file, but does allows you to avoid
  making changes to the .dat file. See the complete documentation for
  information on how to set up a configuration file.
* Finally, ChipEdit has an obscure feature which allows you to control the
  signature of the data file. This is done by adding a SIGNATURE entry to
  the chipedit.ini file. The default signature value is 0x0002AAAC, which
  indicates a data file that uses the MS ruleset. If you set the SIGNATURE
  to be 0x0102AAAC, then ChipEdit will create data files marked to use the
  Lynx ruleset instead.

  Resources on the Internet

There is quite a bit of information about "Chip's Challenge" available on
the internet. Much of it is focused on maximizing your score on the
original level set for the MS game, but you will also find lots of general
help and useful information as well.

Jimmy Vermeer maintains a site that tracks people's scores on the original
level set and CCLP2, as well as information on other levels and links to
many other pages.

  http://www.geocities.com/purpletentacle1977ca/

Anders Kaseorg's site contains the Chip's Challenge FAQ, as well as the AVI
repository and a web interface to the newsgroup:

  http://chips.kaseorg.com/

The grand repository of user-created level sets can be found at the
chips_challenge Yahoo group:

  http://groups.yahoo.com/group/chips_challenge/files/

ChipEdit's home page is at:

  http://www.stage62.com/chipedit/chipedit.htm

Finally, Tile World's home page is at:

  http://www.muppetlabs.com/~breadbox/software/tworld/

  License

Tile World is copyright (C) 2001-2006 by Brian Raiter. This program is free
software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
included in this distribution in the file COPYING, for more details.

  Bugs

Bug reports are always appreciated, and can be sent to the author at
breadbox<at>muppetlabs.com<dot>com. The list of known bugs is at
http://www.muppetlabs.com/~breadbox/software/tworld/BUGS.html. Please check
here before sending a bug report, to to make sure the bug has not already
been documented.

  Credits

Tile World was written by Brian Raiter.

The sound effects included in this distribution were created by Brian
Raiter, with assistance from SoX. Brian Raiter has explictly placed these
files in the public domain.

The tile images included in this distribution were created by Anders
Kaseorg, with assistance from POV-Ray. Anders Kaseorg has explicitly placed
these files in the public domain.

The introductory set of levels included in this distribution were created
by Brian Raiter. Brian Raiter has explictly placed these levels in the
public domain.

"Chip's Challenge" was designed by Chuck Sommerville, who is also the
author of the original Lynx program.

"Chip's Challenge" is a registered trademark of Alpha Omega Publications.

Creating this program would have been flatly impossible without the help of
several fans of "Chip's Challenge". The author would particularly like to
acknowledge Anders Kaseorg for sharing the fruits of his investigations
into the game logic of the MS version and for being an effective bug
hunter, Chuck Sommerville for his pointers regarding the game logic of the
Lynx version and his unfailing support of this project, and "CCExplore" for
his in-depth investigations of esoteric game behavior.

Many other regulars of the annexcafe.chips.challenge newsgroup assisted
with bug reports, suggestions, and all-around encouragement. Their help is
gratefully acknowledged.

The anonymous author of the document describing the .dat file format, Don
Gregory, the "Charter Chipsters", and the contributors to the CC AVI
library all deserve mention as well -- this program would never have been
written without the information they made freely available.

Last but not least, a tip of the hat to John K. Elion for writing ChipEdit.

Brian Raiter
April 2006

==============

-=TileWorld2X Beta 1=-
Copyright (c)2007 Dan Silsby
GNU GPL license, source may be found on
gp2x.de

******BETA RELEASE, BUGS MIGHT BE PRESENT******

Tileworld2X Ported by Dan Silsby 11/07/2007
(Senor Quack on gp32x.com and #gp2xdev on EFNET)
Music files written by chaozz of gp32x.com
and am-fm's music is used under Creative Commons
License.

-=WHAT IS IT?=-

TileWorld is an open source interpreter for
Chip's Challenge levels.  Chip's Challenge remains
a popular game today, despite having been 
introduced in 1989 on the Lynx handheld.  The
version that truly launched Chip's Challenge,
however, was the Microsoft Entertainment Pack
version released in the early nineties. 

Tileworld boasts improved tile graphics and
sound and fluid animation that the MS version
did not.

I have carefully ported TileWorld to the small
screen of the GP2X, and basically rewritten the
entire GUI, as well as a lot of the file handling
code and all of the sound code.  TileWorld2x 
boasts improvements over the desktop version of
TileWorld, including music support, improved
sound samples (OK, one), a simplified and prettier 
GUI, and automated handling of configuration files.

For more info on TileWorld and Chip's Challenge, 
visit the main TileWorld page or Wikipedia:

http://www.muppetlabs.com/~breadbox/software/tworld/
http://en.wikipedia.org/wiki/Chip's_Challenge

NOTICE: The original Chip's Challenge game is 
	NOT OK to redistribute.  If you wish to
	play the original levels from the game,
	(and why wouldn't you?) you will have 
	to copy the original CHIPS.DAT file
	into the data/ folder yourself.  It 
	is best to locate the Windows version.
	TileWorld2x includes over a dozen end-user 
	created	levels, including CCLP2, the 
	unofficial fan-made "sequel" to the 
	original game.  Thus, there is no need 
	to obtain the original game to have some 
	fun. ;)
	
-=OK, WHAT DO I NEED TO KNOW?=-

If you need to learn how to play Chip's Challenge,
there is an introductory level at the top of the
level selection list in TileWorld2X.

You can move Chip about using either the joystick
or the 4 buttons on the right side of the GP2X.

There are thousands of user-created levels
easily found on the Internet:
http://www.pillowpc2001.net/chips/levels.htm
http://www.ecst.csuchico.edu/~pieguy/chips/main.php
http://games.groups.yahoo.com/group/chips_challenge/
http://chips.kaseorg.com/

Some are crappy, some are quite fun.  I have included
some fun ones already.  If you want to
add more to the ones already included, it is very easy.
Simply copy the levelset data file into the data/
folder under tileworld2x.  That's it, just make sure
to unzip it first!  File extension, upper/lower case
do not matter at all, but it is important to
place it in data/, not sets/ and do not place it
in its own subfolder.

Tileworld2x is more intelligent than the original 
TileWorld and will automatically create two 
configuration  files for you, one named
LEVELSETNAME-MS.DAC, and the other 
LEVELSETNAME-LYNX.DAC.  You may now select these
from the main menu.

Why *two* configurations?  Because 
TileWorld is capable of emulating either system, 
Microsoft or Lynx.  Unfortunately, MS changed
some of the allowed behavior in their version 
and so frequently a user-created level 
cannot be played under Lynx mode.  This 
is unfortunate, because Lynx mode allows fluid 
animation and improved sounds.  Since many users
were MS users, you will sometimes have to use
MS mode and forego some fancier graphics in the
name of completing all the levels in a set.

-=CONTACT INFO=-
You can reach me via PM at GP32X.COM forums, or by
email at dansilsby <AT> gmail <DOT> com

Have fun and report bugs, remember this is a 
beta release, anything could happen!


=======================
LIST OF IMRPOVEMENTS OF TILEWORLD FOR HANDHELDS OVER ORIGINAL TILEWORLD:
 (Courtesy of programmer Senor Quack, aka Dan Silsby dansilsby<AT>gmail.com):


* Simplified, menu-driven interface.

* All sprites graphics have been hand-shrunk and optimized for 320x240 screens.  
	The new game interface is reminescent of the Amiga/Lynx versions of Chip's 
	Challenge.  The sprites and blitting code have been modified to allow
	32-bit alpha-blended shadows instead of the ugly checkerboard shadows of
	the original Tileworld.

* Automatic configuration of level packs: No more editing configuration files!
	When first running the program, a folder is created in $HOME/.tworld containing 
	both $HOME/.tworld/data/ and $HOME/.tworld/sets/ folders.  
	
	As with the original PC Tileworld, you can place additional level packs,
	including the CHIPS.DAT file from the original Windows version of Chip's
	Challenge, into the $HOME/.tworld/data/ folder.

	Normally you would need to then manually create configuration files in
	the $HOME/.tworld/sets/ folder for this new levelset, one for running
	with the MS rulset and one for the Lynx rulset.

	Now, the game automatically creates these two configuration files for you
	and there is no need at all to even touch the sets/ folder and I recommend
	you do not.  Additionally, the game will now detect if the original
	CHIPS.DAT file is encountered and automatically add the "fixlynx=y" option
	to its Lynx configuration file (to fix a few minor glitches introduced by
	Microsoft's version of the levelset)
		
* You can control Chip from both the GCW DPAD, the A/B/X/Y buttons, as well 
	as the analog stick (after enabling it from the main menu).

* There is a optional > 30-minute musical soundtrack added from the chipmusic 
	artist Am-Fm (menu music from Chaozz of gp32x.com)

* Automatically remembers last levelpack and level played, so no need to 
	remember and go back to it. Automatically keeps track of scores and times 
	and number of levels in levels completed.

* Improved death sound.  The original death sound was much to distorted and 
	grating for my tastes.

* NOTE: The original Chip's Challenge level pack cannot be included because 
	of copyright restrictions. Instead, a few very good user-created levelpacks 
	have been included, including the best-of levelpacks CCLP2 and CCLP3.  
	A CCLP1 (replacement in spirit for original Chip's Challenge levelpack) 
	level pack is also in the works to be released sometime soon. 

	IF YOU WOULD LIKE TO PLAY THE ORIGINAL CHIP'S CHALLENGE: 
	you can find it by googling the term WITH QUOTES: 
	"Original Chip's Challenge Download".  Unzip the archive and upload the 
	CHIPS.DAT file inside to your $HOME/.tworld/data/ folder via FTP.  

	Many hundreds of community-created levelsets have been created and are
	available for download. The Chip's Challenge community is quite active
	and new levels are created every day.  You can enter your scores in
	online score-tracking sites and compete against the best.

