Console Commands and Command-Line Options
Note: prefix commands with a - or / on the command line (e.g. "-resolution ..." or "/resolution ...")

resolution <width> <height>
default: 640 480
set the window size or screen resolution

depth <bits>
values: 16, 32
default: 32
set the color depth (note: not tested)

fullscreen <enable>
values: 0 (off), 1 (on)
default: off
set full-screen mode

vsync <enable>
values: 0 (off), 1 (on)
default: on
set vertical sync

antialias <enable>
values: 0 (off), 1 (on)
default: off
set OpenGL smoothing for points, lines, and polygons
deprecated, as this does not work for most graphics cards

multisample <samples>
default: 4
set number of multisample samples
try -antialias if this doesn't work (e.g. Radeon Mobility 9000)

viewsize <distance>
default: 320
set view area size in game units

viewaim <distance>
default: 100
set view-aim lookahead distance

viewaimfilter <rate>
default: 2.0
set view-aim filtering rate

input <file>
default: input.xml
set input configuration file to use

level <file>
default: level.xml
set level configuration file to use

record <file>
record player input to the specified file

playback <file>
play back player input from the specified file

simrate <rate>
default: 60
set simulation rate in Hz
rates below 10Hz produce simulation artifacts
rates above monitor refresh produce no benefit
this value cannot be changed at runtime

timescale <scale>
default: 1.0
set time scale factor
scales less than one produce a slow-motion effect
scales greater than one produce a fast-forward effect

motionblur <samples>
default: 1
set number of motion blur samples per frame
note: high levels of motion blur can cause low frame rate

motionblurtime <time>
default: 0.0166666 (1/60th second)
set "shutter speed" for motion blur effect

soundchannels <channels>
default: 8
set the maximum number of sounds that can play at once
setting channels to zero disables the audio mixer

soundvolume <volume>
default: 1.0
set the volume scale

outputconsole <enable>
values: 0 (off), 1 (on)
default: off
send debug trace output to console (viewable with the ` key)

outputdebug <enable>
values: 0 (off), 1 (on)
default: on
send debug trace output to debug (viewable with Visual C++ debugger or Watson)

profilescreen <enable>
values: 0 (off), 1 (on)
display time profile graph on-screen
red = control (AI)
yellow = simulation (calculate and apply forces)
green = physics (move objects, resolve contacts, update joints)
blue = update (links, weapons, spawners)
purple = render (submit draw data)
grey = display (raserization)

profileprint <enable>
values: 0 (off), 1 (on)
send profile output to debug (viewable with Visual C++ debugger or Watson)
C = control (AI)
S = simulation (calculate and apply forces)
P = physics (move objects, resolve contacts, update joints)
U = update (links, weapons, spawners)
R = render (submit draw data)
D = display (rasterization)

frameratescreen <enable>
values: 0 (off), 1 (on)
display minimum, maximum, and average frame rate on-screen

framerateprint <enable>
values: 0 (off), 1 (on)
send minimum, maximum, and average frame rate to debug
times displayed in the format "min<avg<max"

database <databaseid>
display properties of the specified component database
list all database identifiers if no database specified

find <databaseid> <guid>
display properties of component records matching the specified unique identifier
list all component keys in the specified component database if no key specified

components <guid>
list all database identifiers containing a component with specified unique identifier

sound <guid> <cue>
play the specified sound cue associated with the entity matching the specified unique identifier (0 for global)
