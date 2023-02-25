#define kVersion "v0.16"
uint8_t myScreen;

#define ARDUINO 1

////////////////////////////////////////////////////////////////////////////////
// Feature option configuration...

// This enables LOAD, SAVE, FILES commands through the Arduino SD Library
// it adds 9k of usage as well.
#define ENABLE_FILEIO 1

// this turns on "autorun".  if there's FileIO, and a file "autorun.bas",
// then it will load it and run it when starting up
//#define ENABLE_AUTORUN 1
#undef ENABLE_AUTORUN
// and this is the file that gets run
#define kAutorunFilename  "autorun.bas"

// this is the alternate autorun.  Autorun the program in the eeprom.
// it will load whatever is in the EEProm and run it
#define ENABLE_EAUTORUN 1
//#undef ENABLE_EAUTORUN

// this will enable the "TONE", "NOTONE" command using a piezo
// element on the specified pin.  Wire the red/positive/piezo to the kPiezoPin,
// and the black/negative/metal disc to ground.
// it adds 1.5k of usage as well.
#define ENABLE_TONES 1
//#undef ENABLE_TONES
#define kPiezoPin 13
#define TONE_LEDC_CHANNEL (0)

// change keyboard layout
// 0 = US, 1 = UK, 2 = DE,3 = IT,4 = ES
#define keyboard_layout 2

#include "WiFi.h" // include this or you´ll get erro in FabGL
#include "fabgl.h"

fabgl::VGAController VGAController;
fabgl::PS2Controller PS2Controller;
fabgl::Terminal      Terminal;
Canvas cv(&VGAController);
TerminalController tc(&Terminal);

// Sometimes, we connect with a slower device as the console.
// Set your console D0/D1 baud rate here (9600 baud default)
#define kConsoleBaud 115200

////////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
  #ifndef RAMEND
  // okay, this is a hack for now
  // if we're in here, we're a DUE probably (ARM instead of AVR)

  //#define RAMEND 4096-1
  #define RAMEND 16384-1  //-------------------------- RAM increment for ESP32 ---------------------------------------------------
#endif

#include <FS.h>
#include <SPI.h>
#include <SD.h>

/*
 * Connect the SD card to the following pins:
 *
 * SD Card      |       ESP32
*+++++++++++++++++++++++++++++++++++
 * # VDD        |       3.3V
 * # VSS        |       GND
 * # CLK        |       SCK    GPIO18
 * # MISO       |       MISO   GPIO19
 * # MOSI       |       MOSI   GPIO23
 * # CS         |       SS     GPIO05
 */

#define kSD_Fail  0
#define kSD_OK    1

File fp;

// set up our RAM buffer size for program and user input
// NOTE: This number will have to change if you include other libraries.
#ifdef ARDUINO
#ifdef ENABLE_FILEIO
#define kRamFileIO (1030) /* approximate */
#else
#define kRamFileIO (0)
#endif
#ifdef ENABLE_TONES
#define kRamTones (40)
#else
#define kRamTones (0)
#endif
#endif /* ARDUINO */
#define kRamSize  (RAMEND - 1160 - kRamFileIO - kRamTones)

#ifndef ARDUINO
// Not arduino setup
#include <stdio.h>
#include <stdlib.h>
#undef ENABLE_TONES

// size of our program ram
#define kRamSize   4096 /* arbitrary */
//#define kRamSize   16384 /* arbitrary */ //-------------------------------------------------------------- kRamSize -------------------------------------------------------------------------

#ifdef ENABLE_FILEIO
FILE * fp;
#endif
#endif

#ifdef ENABLE_FILEIO
// functions defined elsehwere
void cmd_Files( void );
#endif

////////////////////

#ifndef boolean
#define boolean int
#define true 1
#define false 0
#endif
#endif

#ifndef byte
typedef unsigned char byte;
#endif

// some catches for AVR based text string stuff...
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte( A ) *(A)
#endif

////////////////////

#ifdef ENABLE_FILEIO
unsigned char * filenameWord(void);
static boolean sd_is_initialized = false;
#endif

boolean inhibitOutput = false;
static boolean runAfterLoad = false;
static boolean triggerRun = false;

// these will select, at runtime, where IO happens through for load/save
enum {
  kStreamSerial = 0,
  kStreamEEProm,
  kStreamFile
};
static unsigned char inStream = kStreamSerial;
static unsigned char outStream = kStreamSerial;

////////////////////////////////////////////////////////////////////////////////
// ASCII Characters
#define CR  '\r'
#define NL  '\n'
#define LF  0x0a
#define TAB '\t'
#define BELL  '\b'
#define SPACE   ' '
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC 0x03
#define CTRLH 0x08
#define CTRLS 0x13
#define CTRLX 0x18
#define BACKSPACE 0x7F

typedef short unsigned LINENUM;
#ifdef ARDUINO
#define ECHO_CHARS 1
#else
#define ECHO_CHARS 0
#endif

static unsigned char program[kRamSize];
static const char *  sentinel = "HELLO";
static unsigned char *txtpos,*list_line, *tmptxtpos;
static unsigned char expression_error;
static unsigned char *tempsp;
static long old_millis=0;

/***********************************************************/
// Keyword table and constants - the last character has 0x80 added to it
const static unsigned char keywords[] PROGMEM = {
  'L','I','S','T'+0x80,
  'L','O','A','D'+0x80,
  'N','E','W'+0x80,
  'R','U','N'+0x80,
  'S','A','V','E'+0x80,
  'N','E','X','T'+0x80,
  'L','E','T'+0x80,
  'I','F'+0x80,
  'G','O','T','O'+0x80,
  'G','O','S','U','B'+0x80,
  'R','E','T','U','R','N'+0x80,
  'R','E','M'+0x80,
  'F','O','R'+0x80,
  'I','N','P','U','T'+0x80,
  'P','R','I','N','T'+0x80,
  'P','O','K','E'+0x80,
  'S','T','O','P'+0x80,
  'B','Y','E'+0x80,
  'F','I','L','E','S'+0x80,
  'M','E','M'+0x80,
  '?'+ 0x80,
  '\''+ 0x80,
  'A','W','R','I','T','E'+0x80,
  'D','W','R','I','T','E'+0x80,
  'D','E','L','A','Y'+0x80,
  'E','N','D'+0x80,
  'R','S','E','E','D'+0x80,
  'C','H','A','I','N'+0x80,
  'M','I','L','S'+0x80,
  'C','L','S'+0x80,
  'C','O','L','O','R'+0x80,
  'P','O','I','N','T'+0x80,
  'L','I','N','E'+0x80,
  'R','E','C','T','A','N','G','L','E'+0x80,
  'E','L','I','P','S','E'+0x80,
  'C','U','R','S','O','R'+0x80,
  'A','T'+0x80,
  'I','N','K','E','Y'+0x80,
  'S','C','R','O','L','L'+0x80,
  'C','H','R'+0x80,
  'T','A','B'+0x80,
#ifdef ENABLE_TONES
  'T','O','N','E','W'+0x80,
  'T','O','N','E'+0x80,
  'N','O','T','O','N','E'+0x80,
#endif
  0
};

// by moving the command list to an enum, we can easily remove sections
// above and below simultaneously to selectively obliterate functionality.
enum {
  KW_LIST = 0,
  KW_LOAD, KW_NEW, KW_RUN, KW_SAVE,
  KW_NEXT, KW_LET, KW_IF,
  KW_GOTO, KW_GOSUB, KW_RETURN,
  KW_REM,
  KW_FOR,
  KW_INPUT, KW_PRINT,
  KW_POKE,
  KW_STOP, KW_BYE,
  KW_FILES,
  KW_MEM,
  KW_QMARK, KW_QUOTE,
  KW_AWRITE, KW_DWRITE,
  KW_DELAY,
  KW_END,
  KW_RSEED,
  KW_CHAIN,
  KW_MILS,
  KW_CLS,
  KW_COLOR,
  KW_POINT,
  KW_LINE,
  KW_RECTANGLE,
  KW_ELIPSE,
  KW_CURSOR,
  KW_AT,
  KW_INKEY,
  KW_SCROLL,
  KW_CHR,
  KW_TAB,
#ifdef ENABLE_TONES
  KW_TONEW, KW_TONE, KW_NOTONE,
#endif  
  KW_DEFAULT /* always the final one*/
};

struct stack_for_frame {
  char frame_type;
  char for_var;
  short int terminal;
  short int step;
  unsigned char *current_line;
  unsigned char *txtpos;
};

struct stack_gosub_frame {
  char frame_type;
  unsigned char *current_line;
  unsigned char *txtpos;
};

const static unsigned char func_tab[] PROGMEM = {
  'P','E','E','K'+0x80,
  'G','E','T'+0x80,  
  'A','B','S'+0x80,
  'A','R','E','A','D'+0x80,
  'D','R','E','A','D'+0x80,
  'R','N','D'+0x80,
  'S','I','N'+0x80,
  'C','O','S'+0x80,
  'T','A','N'+0x80,
  'S','G','N'+0x80,
  'I','N','T'+0x80,
  'P','O','W'+0x80,
  'M','I','N'+0x80,
  'M','A','X'+0x80,
  'S','Q','R'+0x80,
  'E','X','P'+0x80,
  'L','O','G'+0x80,
  'L','N'+0x80,
  'R','O','O','T'+0x80,
  'S','Q'+0x80,
  0
};

#define FUNC_PEEK    0
#define FUNC_GET     1
#define FUNC_ABS     2
#define FUNC_AREAD   3
#define FUNC_DREAD   4
#define FUNC_RND     5
#define FUNC_SIN     6
#define FUNC_COS     7
#define FUNC_TAN     8
#define FUNC_SGN     9
#define FUNC_INT     10
#define FUNC_POW     11
#define FUNC_MIN     12
#define FUNC_MAX     13
#define FUNC_SQR     14
#define FUNC_EXP     15
#define FUNC_LOG     16
#define FUNC_LN      17
#define FUNC_ROOT    18
#define FUNC_SQ      19
#define FUNC_UNKNOWN 20

const static unsigned char to_tab[] PROGMEM = {
  'T','O'+0x80,
  0
};

const static unsigned char step_tab[] PROGMEM = {
  'S','T','E','P'+0x80,
  0
};

const static unsigned char relop_tab[] PROGMEM = {
  '>','='+0x80,
  '<','>'+0x80,
  '>'+0x80,
  '='+0x80,
  '<','='+0x80,
  '<'+0x80,
  '!','='+0x80,
  0
};

#define RELOP_GE    0
#define RELOP_NE    1
#define RELOP_GT    2
#define RELOP_EQ    3
#define RELOP_LE    4
#define RELOP_LT    5
#define RELOP_NE_BANG 6
#define RELOP_UNKNOWN 7

const static unsigned char highlow_tab[] PROGMEM = {
  'H','I','G','H'+0x80,
  'H','I'+0x80,
  'L','O','W'+0x80,
  'L','O'+0x80,
  0
};
#define HIGHLOW_HIGH    1
#define HIGHLOW_UNKNOWN 4

#define STACK_SIZE (sizeof(struct stack_for_frame)*5)
#define VAR_SIZE sizeof(short int) // Size of variables in bytes

static unsigned char *stack_limit;
static unsigned char *program_start;
static unsigned char *program_end;
static unsigned char *stack; // Software stack for things that should go on the CPU stack
static unsigned char *variables_begin;
static unsigned char *current_line;
static unsigned char *sp;
#define STACK_GOSUB_FLAG 'G'
#define STACK_FOR_FLAG 'F'
static unsigned char table_index;
static LINENUM linenum;

static const unsigned char okmsg[]            PROGMEM = "OK";
static const unsigned char whatmsg[]          PROGMEM = "What? ";
static const unsigned char howmsg[]           PROGMEM = "How?";
static const unsigned char sorrymsg[]         PROGMEM = "Sorry!";
static const unsigned char initmsg[]          PROGMEM = "TinyBasic Plus " kVersion;
static const unsigned char memorymsg[]        PROGMEM = " bytes free";
static const unsigned char breakmsg[]         PROGMEM = "break!";
static const unsigned char unimplimentedmsg[] PROGMEM = "Unimplemented";
static const unsigned char backspacemsg[]     PROGMEM = "\b\e[K";
static const unsigned char indentmsg[]        PROGMEM = "    ";
static const unsigned char sderrormsg[]       PROGMEM = "SD card error.";
static const unsigned char sdcardmsg[]        PROGMEM = "No SD card.";
static const unsigned char sdfilemsg[]        PROGMEM = "SD file error.";
static const unsigned char dirextmsg[]        PROGMEM = "(dir)";
static const unsigned char slashmsg[]         PROGMEM = "/";
static const unsigned char spacemsg[]         PROGMEM = " ";
static const unsigned char basic[]         PROGMEM = "Tiny Basic ESP32 - ";

static int inchar(void);
static void outchar(unsigned char c);
static void line_terminator(void);
static double expression(void);
static unsigned char breakcheck(void);
static double expression(void);
static void print_info(void);

static double nthroot(double number, double n)
{
  if (n == 0) return NAN;
  if (number > 0) return pow(number, 1.0 / n);
  if (number == 0) return 0;
  if (number < 0 && int(n) == n && (int(n) & 1)) return -pow(-number, 1.0 / n);
  return NAN;
}

/***************************************************************************/
static void ignore_blanks(void) {
  while(*txtpos == SPACE || *txtpos == TAB)
    txtpos++;
}

/***************************************************************************/
static void scantable(const unsigned char *table) {
  int i = 0;
  table_index = 0;
  while(1) {
    // Run out of table entries?
    if(pgm_read_byte( table ) == 0)
      return;

    // Do we match this character?
    if(txtpos[i] == pgm_read_byte( table )) {
      i++;
      table++;
    }
    else {
      // do we match the last character of keywork (with 0x80 added)? If so, return
      if(txtpos[i]+0x80 == pgm_read_byte( table )) {
        txtpos += i+1;  // Advance the pointer to following the keyword
        ignore_blanks();
        return;
      }

      // Forward to the end of this keyword
      while((pgm_read_byte( table ) & 0x80) == 0)
        table++;

      // Now move on to the first character of the next word, and reset the position index
      table++;
      table_index++;
      ignore_blanks();
      i = 0;
    }
  }
}

/***************************************************************************/
static void pushb(unsigned char b) {
  sp--;
  *sp = b;
}

/***************************************************************************/
static unsigned char popb() {
  unsigned char b;
  b = *sp;
  sp++;
  return b;
}

/***************************************************************************/

void printnum(int num)
{
  int digits = 0;

  if(num < 0)
  {
    num = -num;
    outchar('-');
  }
  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

void printUnum(unsigned int num)
{
  int digits = 0;

  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

/***************************************************************************/
static unsigned short testnum(void) {
  unsigned short num = 0;
  ignore_blanks();

  while(*txtpos>= '0' && *txtpos <= '9' ) {
    // Trap overflows
    if(num >= 0xFFFF/10) {
      num = 0xFFFF;
      break;
    }

    num = num *10 + *txtpos - '0';
    txtpos++;
  }
  return  num;
}

/***************************************************************************/
static unsigned char print_quoted_string(void) {
  int i=0;
  unsigned char delim = *txtpos;
  if(delim != '"' && delim != '\'')
    return 0;
  txtpos++;

  // Check we have a closing delimiter
  while(txtpos[i] != delim) {
    if(txtpos[i] == NL)
      return 0;
    i++;
  }

  // Print the characters
  while(*txtpos != delim) {
    outchar(*txtpos);
    txtpos++;
  }
  txtpos++; // Skip over the last delimiter
  return 1;
}

/***************************************************************************/
void printmsgNoNL(const unsigned char *msg) {
  while( pgm_read_byte( msg ) != 0 ) {
    outchar( pgm_read_byte( msg++ ) );
  };
}

/***************************************************************************/
void printmsg(const unsigned char *msg) {
  printmsgNoNL(msg);
  line_terminator();
}

/***************************************************************************/
static void getln(char prompt) {

  outchar(prompt);
  txtpos = program_end+sizeof(LINENUM);

  while(1) {
    char c = inchar();
    switch(c) {
    case NL:
      //break;
    case CR:
      line_terminator();
      // Terminate all strings with a NL
      txtpos[0] = NL;
      return;
    case CTRLH:
    case BACKSPACE:
      if(txtpos == program_end)
        break;
      txtpos--;
      //printmsg(backspacemsg);
      Terminal.write("\b\e[K");
      break;

    default:
      // We need to leave at least one space to allow us to shuffle the line into order
      if(txtpos == variables_begin-2)
        outchar(BELL);
      else {
        txtpos[0] = c;
        txtpos++;
        outchar(c);
      }
    }
  }
}

/***************************************************************************/
static unsigned char *findline(void) {
  unsigned char *line = program_start;
  while(1) {
    if(line == program_end)
      return line;

    if(((LINENUM *)line)[0] >= linenum)
      return line;

    // Add the line length onto the current address, to get to the next line;
    line += line[sizeof(LINENUM)];
  }
}

/***************************************************************************/
static void toUppercaseBuffer(void) {
  unsigned char *c = program_end+sizeof(LINENUM);
  unsigned char quote = 0;

  while(*c != NL) {
    // Are we in a quoted string?
    if(*c == quote)
      quote = 0;
    else if(*c == '"' || *c == '\'')
      quote = *c;
    else if(quote == 0 && *c >= 'a' && *c <= 'z')
      *c = *c + 'A' - 'a';
    c++;
  }
}

/***************************************************************************/
void printline() {
  LINENUM line_num;

  line_num = *((LINENUM *)(list_line));
  list_line += sizeof(LINENUM) + sizeof(char);

  // Output the line */
  printnum(line_num);
  outchar(' ');
  while(*list_line != NL) {
    outchar(*list_line);
    list_line++;
  }
  list_line++;
  line_terminator();
}

/***************************************************************************/
static double expr4(void) {
  // fix provided by Jurg Wullschleger wullschleger@gmail.com
  // fixes whitespace and unary operations
  ignore_blanks();

  if( *txtpos == '-' ) {
    txtpos++;
    return -expr4();
  }
  // end fix

  if(*txtpos == '0') {
    txtpos++;
    return 0;
  }

  if(*txtpos >= '1' && *txtpos <= '9') {
    short int a = 0;
    do  {
      a = a*10 + *txtpos - '0';
      txtpos++;
    }
    while(*txtpos >= '0' && *txtpos <= '9');
    return a;
  }

  // Is it a function or variable reference?
  if(txtpos[0] >= 'A' && txtpos[0] <= 'Z') {
    unsigned char var;
    short int a;
    short int b;
    // Is it a variable reference (single alpha)
    if(txtpos[1] < 'A' || txtpos[1] > 'Z') {
      a = ((short int *)variables_begin)[*txtpos - 'A'];
      txtpos++;
      return a;
    }


    // Is it a function with a single parameter
    scantable(func_tab);
    if(table_index == FUNC_UNKNOWN) {
      goto expr4_error;
    }
    else {

      unsigned char f = table_index;

      ignore_blanks();
      if(*txtpos < 'A' || *txtpos > 'Z')

      if(*txtpos != '(')
        goto expr4_error;

      txtpos++;
      a = expression();

      //double parameter
      if(*txtpos == ',') {
        txtpos++;
        ignore_blanks();

        b = expression();

      } else {
        b = -1;
      }

      if(*txtpos != ')')
        goto expr4_error;
      txtpos++;

      switch(f) {

        case FUNC_PEEK:
        case FUNC_GET:        
          return program[a];

        case FUNC_ABS:
          if(a < 0)
            return -a;
          return a;

        case FUNC_AREAD:
          pinMode( a, INPUT );       
          return analogRead( a );
        case FUNC_DREAD:
          pinMode( a, INPUT );
          return digitalRead( a );

        case FUNC_RND:
          if(b<0){
            return( random( a ));
          } else {
            return( random( a,b ));
          }

        case FUNC_SGN:
          if (a < 0)
              return -1;
          else if (a > 0)
              return 1;
          return a;

        case FUNC_SIN:
          return( sin( a ));

        case FUNC_COS:
          return( cos( a ));

        case FUNC_TAN:
          return( cos( a ));

        case FUNC_INT:
          return( (int)a );

        case FUNC_POW:
          return( pow(a, b) );

        case FUNC_MIN:       
          return( min(a, b) );

        case FUNC_MAX:       
          return( max(a, b) );

        case FUNC_SQR:
           return( sqrt(a) );

        case FUNC_EXP:
           return( exp(a) );

        case FUNC_LOG:
        case FUNC_LN:
        if(b>0){
          return(log (a) / log (b));             
        }    else {
          return( log(a) );
        }    

        case FUNC_ROOT:
          return( nthroot(a,b) );
          
        case FUNC_SQ:
          return( sq(a) );
      }
    }
  }

  if(*txtpos == '('){
    short int a;
    txtpos++;
    a = expression();
    if(*txtpos != ')')
      goto expr4_error;

    txtpos++;
    return a;
  }

expr4_error:
  expression_error = 1;
  return 0;
}

/***************************************************************************/
static double expr3(void) {
  short int a,b;
  a = expr4();
  ignore_blanks(); // fix for eg:  100 a = a + 1

  while(1) {
    if(*txtpos == '*') {
      txtpos++;
      b = expr4();
      a *= b;
    }
    else if(*txtpos == '/') {
      txtpos++;
      b = expr4();
      if(b != 0)
        a /= b;
      else
        expression_error = 1;
    }
    else
      return a;
  }
}

/***************************************************************************/
static double expr2(void) {
  short int a,b;

  if(*txtpos == '-' || *txtpos == '+')
    a = 0;
  else
    a = expr3();

  while(1) {
    if(*txtpos == '-') {
      txtpos++;
      b = expr3();
      a -= b;
    }
    else if(*txtpos == '+') {
      txtpos++;
      b = expr3();
      a += b;
    }
    else
      return a;
  }
}
/***************************************************************************/
static double expression(void) {
  short int a,b;
  a = expr2();

  // Check if we have an error
  if(expression_error)  return a;

  scantable(relop_tab);
  if(table_index == RELOP_UNKNOWN)
    return a;

  switch(table_index) {
  case RELOP_GE:
    b = expr2();
    if(a >= b) return 1;
    break;
  case RELOP_NE:
  case RELOP_NE_BANG:
    b = expr2();
    if(a != b) return 1;
    break;
  case RELOP_GT:
    b = expr2();
    if(a > b) return 1;
    break;
  case RELOP_EQ:
    b = expr2();
    if(a == b) return 1;
    break;
  case RELOP_LE:
    b = expr2();
    if(a <= b) return 1;
    break;
  case RELOP_LT:
    b = expr2();
    if(a < b) return 1;
    break;
  }
  return 0;
}

/***************************************************************************/
void loop() {

  unsigned char *start;
  unsigned char *newEnd;
  unsigned char linelen;
  boolean isDigital = true ,isMax = true;
  boolean alsoWait = false;
  int val;

  program_start = program;
  program_end = program_start;
  sp = program+sizeof(program);  // Needed for printnum
  stack_limit = program+sizeof(program)-STACK_SIZE;
  variables_begin = stack_limit - 27*VAR_SIZE;

warmstart:
  // this signifies that it is running in 'direct' mode.
  current_line = 0;
  sp = program+sizeof(program);
  printmsg(okmsg);

prompt:
  if( triggerRun ){
    triggerRun = false;
    current_line = program_start;
    goto execline;
  }

  getln( '>' );
  toUppercaseBuffer();

  txtpos = program_end+sizeof(unsigned short);

  // Find the end of the freshly entered line
  while(*txtpos != NL)
    txtpos++;

  // Move it to the end of program_memory
  {
    unsigned char *dest;
    dest = variables_begin-1;
    while(1) {
      *dest = *txtpos;
      if(txtpos == program_end+sizeof(unsigned short))
        break;
      dest--;
      txtpos--;
    }
    txtpos = dest;
  }

  // Now see if we have a line number
  linenum = testnum();
  ignore_blanks();
  if(linenum == 0)
    goto direct;

  if(linenum == 0xFFFF)
    goto qhow;

  // Find the length of what is left, including the (yet-to-be-populated) line header
  linelen = 0;
  while(txtpos[linelen] != NL)
    linelen++;
  linelen++; // Include the NL in the line length
  linelen += sizeof(unsigned short)+sizeof(char); // Add space for the line number and line length

  // Now we have the number, add the line header.
  txtpos -= 3;
  *((unsigned short *)txtpos) = linenum;
  txtpos[sizeof(LINENUM)] = linelen;

  // Merge it into the rest of the program
  start = findline();

  // If a line with that number exists, then remove it
  if(start != program_end && *((LINENUM *)start) == linenum) {
    unsigned char *dest, *from;
    unsigned tomove;

    from = start + start[sizeof(LINENUM)];
    dest = start;

    tomove = program_end - from;
    while( tomove > 0) {
      *dest = *from;
      from++;
      dest++;
      tomove--;
    }
    program_end = dest;
  }

  if(txtpos[sizeof(LINENUM)+sizeof(char)] == NL) // If the line has no txt, it was just a delete
    goto prompt;

  // Make room for the new line, either all in one hit or lots of little shuffles
  while(linelen > 0) {
    unsigned int tomove;
    unsigned char *from,*dest;
    unsigned int space_to_make;

    space_to_make = txtpos - program_end;

    if(space_to_make > linelen)
      space_to_make = linelen;
    newEnd = program_end+space_to_make;
    tomove = program_end - start;

    // Source and destination - as these areas may overlap we need to move bottom up
    from = program_end;
    dest = newEnd;
    while(tomove > 0) {
      from--;
      dest--;
      *dest = *from;
      tomove--;
    }

    // Copy over the bytes into the new space
    for(tomove = 0; tomove < space_to_make; tomove++) {
      *start = *txtpos;
      txtpos++;
      start++;
      linelen--;
    }
    program_end = newEnd;
  }
  goto prompt;

unimplemented:
  printmsg(unimplimentedmsg);
  goto prompt;

qhow:
  printmsg(howmsg);
  goto prompt;

qwhat:
  printmsgNoNL(whatmsg);
  if(current_line != NULL) {
    unsigned char tmp = *txtpos;
    if(*txtpos != NL)
      *txtpos = '^';
    list_line = current_line;
    printline();
    *txtpos = tmp;
  }
  line_terminator();
  goto prompt;

qsorry:
  printmsg(sorrymsg);
  goto warmstart;

run_next_statement:
  while(*txtpos == ':')
    txtpos++;
  ignore_blanks();
  if(*txtpos == NL)
    goto execnextline;
  goto interperateAtTxtpos;

direct:
  txtpos = program_end+sizeof(LINENUM);
  if(*txtpos == NL)
    goto prompt;

interperateAtTxtpos:
  if(breakcheck()){
    printmsg(breakmsg);
    goto warmstart;
  }

  scantable(keywords);

  switch(table_index) {
  case KW_DELAY: {
#ifdef ARDUINO
      expression_error = 0;
      val = expression();
      delay( val );
      goto execnextline;
#else
      goto unimplemented;
#endif
    }

  case KW_FILES:
    goto files;
  case KW_LIST:
    goto list;
  case KW_CHAIN:
    goto chain;
  case KW_LOAD:
    goto load;
  case KW_MEM:
    goto mem;
  case KW_NEW:
    if(txtpos[0] != NL)
      goto qwhat;
    program_end = program_start;
    print_info();
    goto prompt;
  case KW_RUN:
    current_line = program_start;
    goto execline;
  case KW_SAVE:
    goto save;
  case KW_NEXT:
    goto next;
  case KW_LET:
    goto assignment;
  case KW_IF:
    short int val;
    expression_error = 0;
    val = expression();
    if(expression_error || *txtpos == NL)
      goto qhow;
    if(val != 0)
      goto interperateAtTxtpos;
    goto execnextline;

  case KW_GOTO:
    expression_error = 0;
    linenum = expression();
    if(expression_error || *txtpos != NL)
      goto qhow;
    current_line = findline();
    goto execline;

  case KW_GOSUB:
    goto gosub;
  case KW_RETURN:
    goto gosub_return;
  case KW_REM:
  case KW_QUOTE:
    goto execnextline;  // Ignore line completely
  case KW_FOR:
    goto forloop;
  case KW_INPUT:
    goto input;
  case KW_PRINT:
  case KW_QMARK:
    goto print;
  case KW_POKE:
    goto poke;
  case KW_MILS:
    goto milis;
  case KW_END:
  case KW_STOP:
    // This is the easy way to end - set the current line to the end of program attempt to run it
    if(txtpos[0] != NL)
      goto qwhat;
    current_line = program_end;
    goto execline;
  case KW_BYE:
    // Leave the basic interperater
    return;

  case KW_AWRITE:  // AWRITE <pin>, HIGH|LOW
    isDigital = false;
    goto awrite;
  case KW_DWRITE:  // DWRITE <pin>, HIGH|LOW
    isDigital = true;
    goto dwrite;

  case KW_RSEED:
    goto rseed;
  case KW_CLS:
    goto cls;
  case KW_COLOR:
    goto color;
  case KW_POINT:
    goto point;
  case KW_LINE:
    goto line;
  case  KW_RECTANGLE:
    goto rectangle;
  case KW_ELIPSE:
    goto elipse;
  case KW_CURSOR:
    goto cursor;
  case KW_AT:
    goto at;
  case KW_SCROLL:
    goto scroll;    
  case KW_CHR:
    goto chr;
  case KW_TAB:
    goto tab;
#ifdef ENABLE_TONES
  case KW_TONEW:
    alsoWait = true;
  case KW_TONE:
    goto tonegen;
  case KW_NOTONE:
    goto tonestop;
#endif

  case KW_DEFAULT:
    goto assignment;
  default:
    break;
  }
scroll:
  tc.setCursorPos(0,0);
  tc.setCursorPos(0,25);
  printmsg((unsigned char *)"");
  tc.setCursorPos(0,0);

tab:
    short int tab;
    ignore_blanks();
    expression_error = 0;
    tab = expression();
    if(expression_error)
      goto qwhat;
    tc.cursorRight(tab);
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
    goto run_next_statement;

chr:
    short int chr;
    ignore_blanks();
    expression_error = 0;
    chr = expression();
    if(expression_error)
      goto qwhat;
    if( chr < 0 || chr > 255)
      goto qwhat;
    Terminal.print(char(chr));
    goto run_next_statement;

execnextline:
  if(current_line == NULL)    // Processing direct commands?
    goto prompt;
  current_line +=  current_line[sizeof(LINENUM)];

execline:
  if(current_line == program_end) // Out of lines to run
    goto warmstart;
  txtpos = current_line+sizeof(LINENUM)+sizeof(char);
  goto interperateAtTxtpos;

milis:
  {
    int value;
    unsigned char var;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
    value = millis() - old_millis;
    old_millis=millis();
    ((short int *)variables_begin)[var - 'A'] = value;
    goto run_next_statement;
  }

input: {
    unsigned char var;
    int value;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
inputagain:
    tmptxtpos = txtpos;
    getln( '?' );
    toUppercaseBuffer();
    txtpos = program_end+sizeof(unsigned short);
    ignore_blanks();
    expression_error = 0;
    value = expression();

    txtpos = tmptxtpos;
    if(expression_error)
      goto inputagain;
    ((short int *)variables_begin)[var-'A'] = value;

    goto run_next_statement;
  }

forloop: {
    unsigned char var;
    short int initial, step, terminal;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != '=')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    expression_error = 0;
    initial = expression();
    if(expression_error)
      goto qwhat;

    scantable(to_tab);
    if(table_index != 0)
      goto qwhat;

    terminal = expression();
    if(expression_error)
      goto qwhat;

    scantable(step_tab);
    if(table_index == 0) {
      step = expression();
      if(expression_error)
        goto qwhat;
    }
    else
      step = 1;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;

    if(!expression_error && *txtpos == NL) {
      struct stack_for_frame *f;
      if(sp + sizeof(struct stack_for_frame) < stack_limit)
        goto qsorry;

      sp -= sizeof(struct stack_for_frame);
      f = (struct stack_for_frame *)sp;
      ((short int *)variables_begin)[var-'A'] = initial;
      f->frame_type = STACK_FOR_FLAG;
      f->for_var = var;
      f->terminal = terminal;
      f->step     = step;
      f->txtpos   = txtpos;
      f->current_line = current_line;
      goto run_next_statement;
    }
  }
  goto qhow;

gosub:
  expression_error = 0;
  linenum = expression();
  if(!expression_error && *txtpos == NL)
  {
    struct stack_gosub_frame *f;
    if(sp + sizeof(struct stack_gosub_frame) < stack_limit)
      goto qsorry;

    sp -= sizeof(struct stack_gosub_frame);
    f = (struct stack_gosub_frame *)sp;
    f->frame_type = STACK_GOSUB_FLAG;
    f->txtpos = txtpos;
    f->current_line = current_line;
    current_line = findline();
    goto execline;
  }
  goto qhow;

next:
  // Fnd the variable name
  ignore_blanks();
  if(*txtpos < 'A' || *txtpos > 'Z')
    goto qhow;
  txtpos++;
  ignore_blanks();
  if(*txtpos != ':' && *txtpos != NL)
    goto qwhat;

gosub_return:
  // Now walk up the stack frames and find the frame we want, if present
  tempsp = sp;
  while(tempsp < program+sizeof(program)-1)
  {
    switch(tempsp[0])
    {
    case STACK_GOSUB_FLAG:
      if(table_index == KW_RETURN)
      {
        struct stack_gosub_frame *f = (struct stack_gosub_frame *)tempsp;
        current_line  = f->current_line;
        txtpos      = f->txtpos;
        sp += sizeof(struct stack_gosub_frame);
        goto run_next_statement;
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_gosub_frame);
      break;
    case STACK_FOR_FLAG:
      // Flag, Var, Final, Step
      if(table_index == KW_NEXT)
      {
        struct stack_for_frame *f = (struct stack_for_frame *)tempsp;
        // Is the the variable we are looking for?
        if(txtpos[-1] == f->for_var)
        {
          short int *varaddr = ((short int *)variables_begin) + txtpos[-1] - 'A';
          *varaddr = *varaddr + f->step;
          // Use a different test depending on the sign of the step increment
          if((f->step > 0 && *varaddr <= f->terminal) || (f->step < 0 && *varaddr >= f->terminal))
          {
            // We have to loop so don't pop the stack
            txtpos = f->txtpos;
            current_line = f->current_line;
            goto run_next_statement;
          }
          // We've run to the end of the loop. drop out of the loop, popping the stack
          sp = tempsp + sizeof(struct stack_for_frame);
          goto run_next_statement;
        }
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_for_frame);
      break;
    default:
      //printf("Stack is stuffed!\n");
      goto warmstart;
    }
  }
  // Didn't find the variable we've been looking for
  goto qhow;

assignment:
  {
    short int value;
    short int *var;


    //Variavel só pode começar com A..Z
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qhow;


    var = (short int *)variables_begin + *txtpos - 'A';

    txtpos++;

    ignore_blanks();

    if (*txtpos != '=')
      goto qwhat;

    txtpos++;
    ignore_blanks();
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
    *var = value;
  }
  goto run_next_statement;
poke:
  {
    short int value;
    unsigned char *address;

    // Work out where to put it
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    address = (unsigned char *)value;

    // check for a comma
    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    // Now get the value to assign
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    //printf("Poke %p value %i\n",address, (unsigned char)value);
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
  }
  goto run_next_statement;

list:
  linenum = testnum(); // Retuns 0 if no line found.

  // Should be EOL
  if(txtpos[0] != NL)
    goto qwhat;

  // Find the line
  list_line = findline();
  while(list_line != program_end)
    printline();
  goto warmstart;

print:
  // If we have an empty list then just put out a NL
  if(*txtpos == ':' )
  {
    line_terminator();
    txtpos++;
    goto run_next_statement;
  }
  if(*txtpos == NL)
  {
    goto execnextline;
  }

  while(1)
  {
    ignore_blanks();
    if(print_quoted_string())
    {
      ;
    }
    else if(*txtpos == '"' || *txtpos == '\'')
      goto qwhat;
    else
    {
      short int e;
      expression_error = 0;
      e = expression();
      if(expression_error)
        goto qwhat;
      printnum(e);
    }

    // At this point we have three options, a comma or a new line
    if(*txtpos == ',')
      txtpos++; // Skip the comma and move onto the next
    else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':'))
    {
      txtpos++; // This has to be the end of the print - no newline
      break;
    }
    else if(*txtpos == NL || *txtpos == ':')
    {
      line_terminator();  // The end of the print statement
      break;
    }
    else
      goto qwhat;
  }
  goto run_next_statement;

mem:
  // memory free
  printnum(variables_begin-program_end);
  printmsg(memorymsg);
  goto run_next_statement;

#ifdef ARDUINO
awrite: // AWRITE <pin>,val
dwrite:
  {
    short int pinNo;
    short int value;
    unsigned char *txtposBak;

    // Get the pin number
    expression_error = 0;
    pinNo = expression();
    if(expression_error)
      goto qwhat;

    // check for a comma
    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    txtposBak = txtpos;
    scantable(highlow_tab);
    if(table_index != HIGHLOW_UNKNOWN) {
      if( table_index <= HIGHLOW_HIGH ) {
        value = 1;
      }
      else {
        value = 0;
      }
    }
    else {

      // and the value (numerical)
      expression_error = 0;
      value = expression();
      if(expression_error)
        goto qwhat;
    }
    pinMode( pinNo, OUTPUT );
    if( isDigital ) {
      digitalWrite( pinNo, value );
    }
    else {
      //analogWrite( pinNo, value );
    }
  }
  goto run_next_statement;
#else
pinmode: // PINMODE <pin>, I/O
awrite: // AWRITE <pin>,val
dwrite:
  goto unimplemented;
#endif

  /*************************************************/
files:
  // display a listing of files on the device.
  // version 1: no support for subdirectories

#ifdef ENABLE_FILEIO
    cmd_Files();
  goto warmstart;
#else
  goto unimplemented;
#endif // ENABLE_FILEIO


chain:
  runAfterLoad = true;

load:
  // clear the program
  program_end = program_start;

  // load from a file into memory
#ifdef ENABLE_FILEIO
  {
    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    // Arduino specific
    if( !SD.exists( (char *)filename ))
    {
      printmsg( sdfilemsg );
    }
    else {

      fp = SD.open( (const char *)filename );
      inStream = kStreamFile;
      inhibitOutput = true;
    }


  }
  goto warmstart;
#else // ENABLE_FILEIO
  goto unimplemented;
#endif // ENABLE_FILEIO


save:
  {

    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

#ifdef ARDUINO
    // remove the old file if it exists
    if( SD.exists( (char *)filename )) {
      SD.remove( (char *)filename );
    }

    // open the file, switch over to file output
    fp = SD.open( (const char *)filename, FILE_WRITE );
        if(!fp){
      Serial.println("Erro na abertura do arquivo");
    }
    outStream = kStreamFile;

    // copied from "List"
    list_line = findline();
    while(list_line != program_end)
      printline();

    // go back to standard output, close the file
    outStream = kStreamSerial;

    fp.close();
#else // ARDUINO
    // desktop
#endif // ARDUINO
    goto warmstart;
  }


rseed:
  {
    short int value;

    //Get the pin number
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;


  randomSeed( value );

    goto run_next_statement;
  }

cls:
{
  Terminal.clear();
  goto run_next_statement;
}


color:
  {
    short int foreColor;
    short int backColor;

    //Get foreColor
    expression_error = 0;
    foreColor = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get the backColor
    expression_error = 0;
    backColor = expression();
    if(expression_error)
      goto qwhat;

    switch (foreColor)
    {
    case 0:
      Terminal.setForegroundColor(fabgl::Color::Red);
      break;
    case 1:
      Terminal.setForegroundColor(fabgl::Color::Green);
      break;
    case 2:
      Terminal.setForegroundColor(fabgl::Color::Yellow);
      break;
    case 3:
      Terminal.setForegroundColor(fabgl::Color::Blue);
      break;
    case 4:
      Terminal.setForegroundColor(fabgl::Color::Magenta);
      break;
    case 5:
      Terminal.setForegroundColor(fabgl::Color::Cyan);
      break;
    case 6:
      Terminal.setForegroundColor(fabgl::Color::White);
      break;
    case 7:
      Terminal.setForegroundColor(fabgl::Color::BrightBlack);
      break;
    case 8:
      Terminal.setForegroundColor(fabgl::Color::BrightRed);
      break;
    case 9:
      Terminal.setForegroundColor(fabgl::Color::BrightGreen);
      break;
    case 10:
      Terminal.setForegroundColor(fabgl::Color::BrightYellow);
      break;
    case 11:
      Terminal.setForegroundColor(fabgl::Color::BrightBlue);
      break;
    case 12:
      Terminal.setForegroundColor(fabgl::Color::BrightMagenta);
      break;
    case 13:
      Terminal.setForegroundColor(fabgl::Color::BrightCyan);
      break;
    case 14:
      Terminal.setForegroundColor(fabgl::Color::BrightWhite);
      break;
    case 15:
      Terminal.setForegroundColor(fabgl::Color::Black);
      break;
    default:
      Terminal.setForegroundColor(fabgl::Color::BrightBlack);
      break;

    }
    
    switch (backColor)
    {
    case 0:
      Terminal.setBackgroundColor(fabgl::Color::Red);
      break;
    case 1:
      Terminal.setBackgroundColor(fabgl::Color::Green);
      break;
    case 2:
      Terminal.setBackgroundColor(fabgl::Color::Yellow);
      break;
    case 3:
      Terminal.setBackgroundColor(fabgl::Color::Blue);
      break;
    case 4:
      Terminal.setBackgroundColor(fabgl::Color::Magenta);
      break;
    case 5:
      Terminal.setBackgroundColor(fabgl::Color::Cyan);
      break;
    case 6:
      Terminal.setBackgroundColor(fabgl::Color::White);
      break;
    case 7:
      Terminal.setBackgroundColor(fabgl::Color::BrightBlack);
      break;
    case 8:
      Terminal.setBackgroundColor(fabgl::Color::BrightRed);
      break;
    case 9:
      Terminal.setBackgroundColor(fabgl::Color::BrightGreen);
      break;
    case 10:
      Terminal.setBackgroundColor(fabgl::Color::BrightYellow);
      break;
    case 11:
      Terminal.setBackgroundColor(fabgl::Color::BrightBlue);
      break;
    case 12:
      Terminal.setBackgroundColor(fabgl::Color::BrightMagenta);
      break;
    case 13:
      Terminal.setBackgroundColor(fabgl::Color::BrightCyan);
      break;
    case 14:
      Terminal.setBackgroundColor(fabgl::Color::BrightWhite);
      break;
    case 15:
      Terminal.setBackgroundColor(fabgl::Color::Black);
      break;
    default:
      Terminal.setBackgroundColor(fabgl::Color::Black);
      break;
    }
    goto run_next_statement;
  }

point: {
  short int color;
  short int x;
  short int y;

    //Get color
    expression_error = 0;
    color = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get X
    expression_error = 0;
    x = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get y
    expression_error = 0;
    y = expression();
    if(expression_error)
      goto qwhat;

    setPenColor(color);
    cv.setPixel(x,y);

  goto run_next_statement;
}

line: {
  short int color;
  short int startX;
  short int startY;
  short int endX;
  short int endY;
  short int penWidth;
    //Get color
    expression_error = 0;
    color = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get startX
    expression_error = 0;
    startX = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get startY
    expression_error = 0;
    startY = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get endX
    expression_error = 0;
    endX = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get endY
    expression_error = 0;
    endY = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get pen width
    expression_error = 0;
    penWidth = expression();
    if(expression_error)
      goto qwhat;

    setPenColor(color);
    cv.setPenWidth(penWidth);
    cv.drawLine(startX, startY, endX, endY);


  goto run_next_statement;
}

rectangle: {
  short int color;
  short int fillColor;
  short int startX;
  short int startY;
  short int endX;
  short int endY;
  short int penWidth;

    //Get color
    expression_error = 0;
    color = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get fillColor
    expression_error = 0;
    fillColor = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get startX
    expression_error = 0;
    startX = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get startY
    expression_error = 0;
    startY = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get endX
    expression_error = 0;
    endX = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get endY
    expression_error = 0;
    endY = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get pen width
    expression_error = 0;
    penWidth = expression();
    if(expression_error)
      goto qwhat;

    setPenColor(color);
    cv.setPenWidth(penWidth);
    cv.drawRectangle(startX, startY, endX, endY);

    cv.setPenWidth(1);
    //Fills the retangle
    if(fillColor > -1) {

      setPenColor(fillColor);
      for(short int y = startY+ 1; y < endY; y++ ){
        cv.drawLine(startX +1 , y, endX-1, y);
      }

    }

  goto run_next_statement;
}

elipse: {
  short int color;
  short int x;
  short int y;
  short int width;
  short int height;
  short int penWidth;

    //Get color
    expression_error = 0;
    color = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get x
    expression_error = 0;
    x = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get y
    expression_error = 0;
    y = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get width
    expression_error = 0;
    width = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get height
    expression_error = 0;
    height = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get penWidth
    expression_error = 0;
    penWidth = expression();
    if(expression_error)
      goto qwhat;

    setPenColor(color);
    cv.setPenWidth(penWidth);

    cv.drawEllipse(x, y, width, height);


  goto run_next_statement;
}

cursor: {
  short int enable;
    //Get enable
    expression_error = 0;
    enable = expression();
    if(expression_error)
      goto qwhat;

    Terminal.enableCursor(enable);

  goto run_next_statement;
}

at: {
    short int x;
    short int y;
      //Get X
    expression_error = 0;
    x = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();


    //Get height
    expression_error = 0;
    y = expression();
    if(expression_error)
      goto qwhat;

    tc.setCursorPos(x,y);
    goto run_next_statement;
}


#ifdef ENABLE_TONES
tonestop:
  noTone( kPiezoPin );
  goto run_next_statement;

tonegen: {
    // TONE freq, duration
    // if either are 0, tones turned off
    short int freq;
    short int duration;

    //Get the frequency
    expression_error = 0;
    freq = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    //Get the duration
    expression_error = 0;
    duration = expression();
    if(expression_error)
      goto qwhat;

    if( freq == 0 || duration == 0 )
      goto tonestop;

    tone( kPiezoPin, freq, duration );
    if( alsoWait ) {
      delay( duration );
      alsoWait = false;
    }
    goto run_next_statement;
  }
#endif /* ENABLE_TONES */
}

// returns 1 if the character is valid in a filename
static int isValidFnChar( char c ){
  if( c >= '0' && c <= '9' ) return 1; // number
  if( c >= 'A' && c <= 'Z' ) return 1; // LETTER
  if( c >= 'a' && c <= 'z' ) return 1; // letter (for completeness)
  if( c == '/' ) return 1;
  if( c == '_' ) return 1;
  if( c == '+' ) return 1;
  if( c == '.' ) return 1;
  if( c == '~' ) return 1;  // Window~1.txt

  return 0;
}

unsigned char * filenameWord(void) {
  // SDL - I wasn't sure if this functionality existed above, so I figured i'd put it here
  unsigned char * ret = txtpos;
  expression_error = 0;

  // make sure there are no quotes or spaces, search for valid characters
  //while(*txtpos == SPACE || *txtpos == TAB || *txtpos == SQUOTE || *txtpos == DQUOTE ) txtpos++;
  while( !isValidFnChar( *txtpos )) txtpos++;
  ret = txtpos;

  if( *ret == '\0' ) {
    expression_error = 1;
    return ret;
  }

  // now, find the next nonfnchar
  txtpos++;
  while( isValidFnChar( *txtpos )) txtpos++;
  if( txtpos != ret ) *txtpos = '\0';

  // set the error code if we've got no string
  if( *ret == '\0' ) {
    expression_error = 1;
  }

  return ret;
}

static void tone(uint8_t pin, unsigned int frequency, uint8_t duration) {
  ledcAttachPin(pin, TONE_LEDC_CHANNEL);
  ledcWriteTone(TONE_LEDC_CHANNEL, frequency);
  delay(duration);
  noTone(pin);
}

static void noTone(uint8_t pin){
  ledcDetachPin(pin);
  ledcWrite(TONE_LEDC_CHANNEL, 0);
}


/***************************************************************************/
static void line_terminator(void) {
  outchar(NL);
  outchar(CR);
}

/***********************************************************/
void setup() {
#ifdef ARDUINO
  Serial.begin(kConsoleBaud); // opens serial port
  while( !Serial ); // for Leonardo

  //Serial.println("Keyboard Test:");
  delay(500);  // avoid garbage into the UART
  Serial.write("\n\nReset\n");

  PS2Controller.begin(PS2Preset::KeyboardPort0);

  auto keyboard = PS2Controller.keyboard();
  switch (keyboard_layout) {// 0 = US, 1 = UK, 2 = DE,3 = IT,4 = ES
    case 0:
      keyboard->setLayout(&fabgl::USLayout);
      break;
    case 1:
      keyboard->setLayout(&fabgl::UKLayout);
      break;
    case 2:
      keyboard->setLayout(&fabgl::GermanLayout);
      break;
    case 3:
      keyboard->setLayout(&fabgl::ItalianLayout);
      break;
    case 4:
      keyboard->setLayout(&fabgl::SpanishLayout);
      break;
    default:
      keyboard->setLayout(&fabgl::USLayout);
      break;
  }

  VGAController.begin(GPIO_NUM_22, GPIO_NUM_15, GPIO_NUM_21, GPIO_NUM_17, GPIO_NUM_4);
  VGAController.setResolution(VGA_640x200_70Hz);
  //VGAController.setResolution(VGA_640x480_60Hz);
/*
  VGAController.setPaletteItem(0, RGB888(0, 0, 0));
  VGAController.setPaletteItem(1, RGB888(255, 255, 255));
  VGAController.setPaletteItem(2, RGB888(104, 55, 43));
  VGAController.setPaletteItem(3, RGB888(122, 164, 178));
  VGAController.setPaletteItem(4, RGB888(111, 61, 134));
  VGAController.setPaletteItem(5, RGB888(88, 141, 67));
  VGAController.setPaletteItem(6, RGB888(53, 40, 121));
  VGAController.setPaletteItem(7, RGB888(0, 0, 0));
  VGAController.setPaletteItem(8, RGB888(0, 0, 0));
  VGAController.setPaletteItem(9, RGB888(0, 0, 0));
  VGAController.setPaletteItem(10, RGB888(0, 0, 0));
  VGAController.setPaletteItem(11, RGB888(0, 0, 0));
  VGAController.setPaletteItem(12, RGB888(0, 0, 0));
  VGAController.setPaletteItem(13, RGB888(0, 0, 0));
  VGAController.setPaletteItem(14, RGB888(0, 0, 0));
  VGAController.setPaletteItem(15, RGB888(0, 0, 0));
  VGAController.setPaletteItem(16, RGB888(0, 0, 0));

		 * #000000 black      0, RGB888(000, 000, 000)
		 * #FFFFFF white      1, RGB888(255, 255, 255)
		 * #68372B red        2, RGB888(104, 055, 043)
		 * #70A4B2 light blue 3, RGB888(122, 164, 178)
		 * #6F3D86 purple     4, RGB888(111, 061, 134)
		 * #588D43 green      5, RGB888(088, 141, 067)
		 * #352879 dark blue  6, RGB888(053, 040, 121)
		 * #B8C76F yellow
		 * #6F4F25 brown
		 * #433900 dark brown
		 * #9A6759 light red
		 * #444444 dark grey
		 * #6C6C6C mid grey
		 * #9AD284 light green
		 * #6C5EB5 mid blue
		 * #959595 light grey
*/  

  Canvas cv(&VGAController);
  Terminal.begin(&VGAController);
  Terminal.connectLocally();      // to use Terminal.read(), available(), etc..
  print_info();

#ifdef ENABLE_FILEIO
  initSD();

#ifdef ENABLE_AUTORUN
  if( SD.exists( kAutorunFilename )) {
    program_end = program_start;
    fp = SD.open( kAutorunFilename );
    inStream = kStreamFile;
    inhibitOutput = true;
    runAfterLoad = true;
  }
#endif /* ENABLE_AUTORUN */

#endif /* ENABLE_FILEIO */

#endif /* ARDUINO */
}

static void print_info(){   
    Terminal.clear();    
    Terminal.printf("Tiny Basic ESP32 %s\r\n", kVersion);
    Terminal.printf("\e[31mScreen Size        :\e[33m %d x %d\r\n", VGAController.getScreenWidth(), VGAController.getScreenHeight());
    Terminal.printf("\e[32mTerminal Size      :\e[33m %d x %d\r\n", Terminal.getColumns(), Terminal.getRows());
    Terminal.printf("\e[34mCPU                :\e[33m %d MHz\r\n", getCpuFrequencyMhz());
    Terminal.printf("\e[35mFree DMA Memory    :\e[33m %d\r\n", heap_caps_get_free_size(MALLOC_CAP_DMA));
    Terminal.printf("\e[36mFree 32 bit Memory :\e[33m %d\r\n\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    tone( kPiezoPin, 4400, 100 );
    Terminal.setForegroundColor(Color::White);    
    Terminal.enableCursor(1);    
}

/***********************************************************/
static unsigned char breakcheck(void)
{
#ifdef ARDUINO
  if(Serial.available())
    return Serial.read() == CTRLC;
  return 0;
#else
#ifdef __CONIO__
  if(kbhit())
    return getch() == CTRLC;
  else
#endif
    return 0;
#endif
}
/***********************************************************/
static int inchar()
{
  int v;
#ifdef ARDUINO

  switch( inStream ) {
  case( kStreamFile ):
#ifdef ENABLE_FILEIO
    v = fp.read();
    if( v == NL ) v=CR; // file translate
    if( !fp.available() ) {
      fp.close();
      goto inchar_loadfinish;
    }
    return v;
#else
#endif
     break;
  case( kStreamEEProm ):
    inStream = kStreamSerial;
    return NL;
     break;
  case( kStreamSerial ):
  default:
    while(1)
    {

     //----------- the following is the key modification -------------------------------------------------------------------------------------------------
     //----------- where the code get the variables from the PS2 keyboard --------------------------------------------------------------------------------
     //----------- and treat them as the ones from the PC keyboard ---------------------------------------------------------------------------------------

     /*if (keyboard.available()) {
       // read the next key
       char c = keyboard.read();
       //Serial.print(c);
       return c;
     }*/
     if (Terminal.available()) {
       // read the next key
       char c = Terminal.read();
       //myWrite(c);
       //Serial.print(c);
       return c;
     }
     //------------ end of modification -------------------------------------------------------------------------------------------------------------------

     /*if(Serial.available())
     return Serial.read(); */
      if(Serial.available()){
         char c = Serial.read();
         //vga.print(c); //-------------------------------------------------- così scrive solo i caratteri della tastiera ----------------------------------------------------------------
         return c;
      }
    }
  }

inchar_loadfinish:
  inStream = kStreamSerial;
  inhibitOutput = false;

  if( runAfterLoad ) {
    runAfterLoad = false;
    triggerRun = true;
  }
  return NL; // trigger a prompt.

#else
  // otherwise. desktop!
  int got = getchar();

  // translation for desktop systems
  if( got == LF ) got = CR;

  return got;
#endif
}

/***********************************************************/
static void outchar(unsigned char c)
{
  if( inhibitOutput ) return;

#ifdef ARDUINO
  #ifdef ENABLE_FILEIO
    if( outStream == kStreamFile ) {
      // output to a file
      fp.write( c );
    }
    else
  #endif
    Serial.write(c);
    Terminal.write(c); //------------------------ here to write to the VGA monitor -----------------------------------
    myWrite(c);
#else
  putchar(c);
#endif
}

/***********************************************************/
/* SD Card helpers */

#if ARDUINO && ENABLE_FILEIO

static int initSD(void){
  // if the card is already initialized, we just go with it.
  // there is no support (yet?) for hot-swap of SD Cards. if you need to
  // swap, pop the card, reset the arduino.)
  if( sd_is_initialized == true ) return kSD_OK;

  if (!SD.begin(SS)) {
    printmsg( sderrormsg );
    return kSD_Fail;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE){
    printmsg( sdcardmsg );
    return kSD_Fail;
  }

  // success - quietly return 0
  sd_is_initialized = true;

  // and our file redirection flags
  outStream = kStreamSerial;
  inStream = kStreamSerial;
  inhibitOutput = false;

  return kSD_OK;
}
#endif

#if ENABLE_FILEIO
void cmd_Files(void) {
  File dir = SD.open( "/" );
  dir.seek(0);

  while(true) {
    File entry = dir.openNextFile();
    if(!entry) {
      entry.close();
      break;
    }

    // common header
    printmsgNoNL(indentmsg);
    printmsgNoNL((const unsigned char *)entry.name());
    if(entry.isDirectory()) {
      printmsgNoNL(slashmsg);
    }

    if( entry.isDirectory() ) {
      // directory ending
      for( int i=strlen( entry.name()) ; i<16 ; i++ ) {
        printmsgNoNL(spacemsg);
      }
      printmsgNoNL(dirextmsg);
    }
    else {
      // file ending
      for( int i=strlen( entry.name()) ; i<17 ; i++ ) {
        printmsgNoNL( spacemsg );
      }
      printUnum( entry.size() );
    }
    line_terminator();
    entry.close();
  }
  dir.close();
}
#endif

void myWrite(char c) {
  if (Terminal.available()) {
    c = Terminal.read();
    switch (c) {
     case 0x7F:       // DEL -> backspace + ESC[K
       Terminal.write("\b\e[K");
       break;
     case 0x0D:       // CR  -> CR + LF
       Terminal.write("\r\n");
       break;
     case 0x03:       // ctrl+c
       printmsg(breakmsg);
       current_line = 0;
       sp = program+sizeof(program);
       printmsg(okmsg);
       break;
     case 0x02:       // ctrl+b
       Terminal.setForegroundColor(Color::Red);
       //Terminal.printf("\e[35m");
       break;
     case 0x1B:       // ESC
       //Terminal.write("\r\n");
       switch (myScreen){
          case 0:
             Terminal.setBackgroundColor(Color::Black);
             Terminal.setForegroundColor(Color::Yellow);
             myScreen = 1;
          break;
          case 1:
             Terminal.setBackgroundColor(Color::Black);
             Terminal.setForegroundColor(Color::BrightWhite);
             myScreen = 2;
          break;
          case 2:
             Terminal.setBackgroundColor(Color::Blue);
             Terminal.setForegroundColor(Color::White);
             myScreen = 3;
          break;
          case 3:
             Terminal.setBackgroundColor(Color::Black);
             Terminal.setForegroundColor(Color::BrightGreen);
             myScreen = 0;
          break;
       }
       break;
    default:
       Terminal.write(c);
       break;
    }
  }
}

void setPenColor(int color){
    switch (color){
    case 0:
      cv.setPenColor(fabgl::Color::Red);
      break;
    case 1:
      cv.setPenColor(fabgl::Color::Green);
      break;
    case 2:
      cv.setPenColor(fabgl::Color::Yellow);
      break;
    case 3:
      cv.setPenColor(fabgl::Color::Blue);
      break;
    case 4:
      cv.setPenColor(fabgl::Color::Magenta);
      break;
    case 5:
      cv.setPenColor(fabgl::Color::Cyan);
      break;
    case 6:
      cv.setPenColor(fabgl::Color::White);
      break;
    case 7:
      cv.setPenColor(fabgl::Color::BrightBlack);
      break;
    case 8:
      cv.setPenColor(fabgl::Color::BrightRed);
      break;
    case 9:
      cv.setPenColor(fabgl::Color::BrightGreen);
      break;
    case 10:
      cv.setPenColor(fabgl::Color::BrightYellow);
      break;
    case 11:
      cv.setPenColor(fabgl::Color::BrightBlue);
      break;
    case 12:
      cv.setPenColor(fabgl::Color::BrightMagenta);
      break;
    case 13:
      cv.setPenColor(fabgl::Color::BrightCyan);
      break;
    case 14:
      cv.setPenColor(fabgl::Color::BrightWhite);
      break;
    case 15:
      cv.setPenColor(fabgl::Color::Black);
      break;
    default:
      cv.setPenColor(fabgl::Color::BrightBlack);
      break;
    }
}
