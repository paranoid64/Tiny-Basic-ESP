v0.16: 2023-02-07

add keyboad layout switch
0 = US, 1 = UK, 2 = DE,3 = IT,4 = ES
e.g.: #define keyboard_layout 2 DE


add commands:
SCROLL
CHR <int dec>
TAB <int>
SGN(<int>) (Signumfunction)
INT(<double>)
sin(<int rad>)
con(<int rad>)
tan(<int rad>)

v0.15: 2029-10-23
Changes by fg1998 (fg1998@gmail.com)
Only for ESP32 with FabGL !!!!!!
Modified for latest version of FagGL (0.9.0)
Redefined SPI Pins for SD Card work with VGA32_V1.4 from TTGO - SCK = 14,  MISO = 02, MOSI = 12, CS = 13
Backspace is now working with Terminal
LOAD and SAVE working,  needs to start with a '/' ex: LOAD "/test.bas"
Adjust screen resolution to 640x200 (80x25 text) for low ram use
**NEW COMMANDS
CLS
COLOR <INT>,<INT> -> FORECOLOR, BACKCOLOR
POINT <INT>, <INT>, <INT> -> COLOR, X, Y
LINE <INT>, <INT>, <INT>, <INT>, <INT>, <INT>  -> COLOR, INIT X, INIT Y, END X, END Y, PEN WIDTH
RECTANGLE <INT>, <INT>, <INT>, <INT>, <INT>, <INT>, <INT> -> COLOR, COLOR FILL (-1 USES NO COLOR), INIT X, INIT Y, END X, END Y, PEN WIDTH
ELIPSE <INT>, <INT>, <INT>, <INT>, <INT> -> COLOR, X, Y, WIDTH, HEIGHT, PEN WIDTH
CURSOR 0/1 -> ENABLE/DISABLE CURSOR
AT <INT>,<INT> -> puts cursor on x,y

v0.14: 2013-11-07
Input command always set the variable to 99
Modified Input command to accept an expression using getn()
Syntax is "input x" where x is any variable

v0.13: 2013-03-04
Support for Arduino 1.5 (SPI.h included, additional changes for DUE support)

v0.12: 2013-03-01
EEPROM load and save routines added: EFORMAT, ELIST, ELOAD, ESAVE, ECHAIN
added EAUTORUN option (chains to EEProm saved program on startup)
Bugfixes to build properly on non-arduino systems (PROGMEM #define workaround)
cleaned up a bit of the #define options wrt TONE

v0.11: 2013-02-20
all display strings and tables moved to PROGMEM to save space
removed second serial
removed pinMode completely, autoconf is explicit
beginnings of EEPROM related functionality (new,load,save,list)

v0.10: 2012-10-15
added kAutoConf, which eliminates the "PINMODE" statement.
now, DWRITE,DREAD,AWRITE,AREAD automatically set the PINMODE appropriately themselves.
should save a few bytes in your programs.

v0.09: 2012-10-12
Fixed directory listings.  FILES now always works. (bug in the SD library)
ref: http:arduino.cc/forum/index.php/topic,124739.0.html
fixed filesize printouts (added printUnum for unsigned numbers)
#defineable baud rate for slow connection throttling
e
v0.08: 2012-10-02
Tone generation through piezo added (TONE, TONEW, NOTONE)

v0.07: 2012-09-30
Autorun buildtime configuration feature

v0.06: 2012-09-27
Added optional second serial input, used for an external keyboard

v0.05: 2012-09-21
CHAIN to load and run a second file
RND,RSEED for random stuff
Added "!=" for "<>" synonym
Added "END" for "STOP" synonym (proper name for the functionality anyway)

v0.04: 2012-09-20
DELAY ms   - for delaying
PINMODE <pin>, INPUT|IN|I|OUTPUT|OUT|O
DWRITE <pin>, HIGH|HI|1|LOW|LO|0
AWRITE <pin>, [0..255]
fixed "save" appending to existing files instead of overwriting
Updated for building desktop command line app (incomplete)

v0.03: 2012-09-19
Integrated Jurg Wullschleger whitespace,unary fix
Now available through github
Project renamed from "Tiny Basic in C" to "TinyBasic Plus"

v0.02b: 2012-09-17  Scott Lawrence <yorgle@gmail.com>
Better FILES listings

v0.02a: 2012-09-17  Scott Lawrence <yorgle@gmail.com>
Support for SD Library
Added: SAVE, FILES (mostly works), LOAD (mostly works) (redirects IO)
Added: MEM, ? (PRINT)
Quirk:  "10 LET A=B+C" is ok "10 LET A = B + C" is not.
Quirk:  INPUT seems broken?
