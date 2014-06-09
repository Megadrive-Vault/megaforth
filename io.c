#include "genesis.h"
#include "string.h"
#include "vdp.h"
#include "vdp_bg.h"
#include "io.h"
#include "KDebug.h"


extern char buffer[WIDTH_TILES];
extern u32 *bufftop, *curkey;

static int printCol, printRow;

static int height_tiles = HEIGHT_TILES;
static int height_input = HEIGHT_INPUT_NO_OSK;

static u16 inputPlan;
static u16 oskPlan;
static int displayingOsk = 0;
static u16 inputFlags;
static u16 oskFlags;



void drawOsk();
void initCursorSprite();

void initIO() {
  inputPlan = APLAN;
  inputFlags = TILE_ATTR(PAL0,0,0,0);
  
  oskPlan = BPLAN;
  oskFlags = TILE_ATTR(PAL2,0,0,0);
    
  VDP_init();
  JOY_init();
  initCursorSprite();
  printCol = 1;
  printRow = 0;
  VDP_setTextPlan(inputPlan);
}

void waitForStart() {
  JOY_waitPressBtn(JOY_1, BUTTON_START);  
}

void clearScreen() {
  VDP_clearPlan(inputPlan, 1);
  VDP_clearPlan(oskPlan, 1);
  
  if(displayingOsk == 1) {
    drawOsk();
  }
  VDP_waitVSync();
}

void scrollUp() {

}

void scrollDown() {

}

void incPrintRow() {
  printRow++;
  printCol = 1;
  if(printRow >= height_input) {
    clearScreen();
    printRow = 0;
    printCol = 1;
  }
  
}

void incPrintCol() {
  printCol++;
  if(printCol >= WIDTH_TILES+1) {
    
    incPrintRow();
    
  }
}

static int cursorCnt = 0;
int printCursor() {
  int retval = 0;
  
  cursorCnt++;
  
  if(cursorCnt >= 20 && cursorCnt <= 40) {
    retval = 1;
  }

  if(cursorCnt > 40) {
    cursorCnt = 0;
  }
  return retval;
}

char charUpcase(char c) {
  if((c >= 97) & (c <= 122)) {
    return c-32;
  } else {
    return c;
  }
}

char charDowncase(char c) {
  if((c >= 41) & (c <= 90)) {
    return c+32;
  } else {
    return c;
  }
}


extern u32 **loc_latest;
static u32 *cur_entry;
static int prefix_cnt, clear_cnt, continue_match;


void resetPrefixSearch() {
  cur_entry = *loc_latest;
  prefix_cnt = 0;
  clear_cnt = 0;
  continue_match = 0;
}



// returns a pointer to the next entry that matches the given prefix
u32* prefixMatch(char* word, u32 wordLen) {
  u8 nameLen;
  char* entryWord;
  
  if(!continue_match) {
    cur_entry = loc_latest;
    continue_match = 1;
  }
  
 check_match:
  nameLen = (*(((u8*)cur_entry)+4));
  
  if((0x1F & nameLen) < wordLen) {
    // continue onto next entry
    goto get_next;
  } else {
    entryWord = ((u8*)cur_entry)+5;
    // try to match characters
    for(int i = 0; i < wordLen; i++) {
      if(entryWord[i] != word[i]) {
        goto get_next;
      }
    }
    u32* res_entry = cur_entry;
    cur_entry = *((u32**)cur_entry);
    return res_entry;
    
  }
  
 get_next:
  // dereference link
  cur_entry = *((u32**)cur_entry);
  if(cur_entry) {
    goto check_match; 
  } else {
    goto no_match;
  }
  
 no_match:
  cur_entry = *loc_latest;
  return NULL;

}

int oskX, oskY;
void drawOsk() {
  height_input = HEIGHT_INPUT_OSK;
  
  //VDP_setTextPlan(oskPlan);
  
  u8 ascii_cnt = CHAR_START;
  char str[2] = {' ', '\0'};
  for(int y = height_input; y < height_tiles; y+=2) {
    for(int x = 1; x < WIDTH_TILES; x+=2) {
      str[0] = ascii_cnt++;
      VDP_drawTextBG(oskPlan, str, oskFlags, x, y);
      if(ascii_cnt >= 128) { return; }
    }
  }

  if(printRow >= height_input) {
    clearScreen();
    printRow = 0;
    printCol = 1;
  }
}

static u16 cursorFlags = TILE_ATTR_FULL(PAL0,1,0,0,1);
void clearOsk() {
  VDP_clearPlan(oskPlan, 1);
  height_input = HEIGHT_INPUT_NO_OSK;
  VDP_setSprite(0, 0, 0, SPRITE_SIZE(2,2), cursorFlags, 0);
  VDP_updateSprites();
  VDP_waitVSync();
}



// 4x4 is base tile size
const u32 spriteTiles[4*8] =
  {
    
    0x00800000,
    0x00880000,
    
    0x00888000,
    0x00888800,  
    
    0x00888880,
    0x00888888,
    
    0x00888800,
    0x00800000
  };

void initCursorSprite() {
  // load into position 1? in vram
  // data, vram offset, num_tiles, use_dma
  VDP_loadTileData((const u32*)spriteTiles, 1, 4, 0);
}

void drawOskCursor() {
  int x = (1+(oskX * 2)) * 8;
  int y = (HEIGHT_INPUT_OSK + (oskY * 2)) * 8;
  // update position
  VDP_setSprite(0, x+4, y+6, SPRITE_SIZE(2,2), cursorFlags, 0);
  // draw
  VDP_updateSprites();
}
      




static int ptr;
static int minPtr = 0;
static char tmpBuffer[WIDTH_TILES+1];
static int defMode;

void incPtr(){
  ptr++;
  if (ptr >= WIDTH_TILES) {
    ptr = minPtr;
  }
}

void decPtr() {
  ptr--;
  if(ptr < minPtr) {
    ptr = WIDTH_TILES-1;
  }
}
  
void shiftBack(int start) {
  for(int i = start; i < WIDTH_TILES-1; i++) {
    tmpBuffer[i] = tmpBuffer[i+1];
  }
} 
  
void shiftForward(int start) {
  for(int i = WIDTH_TILES-2; i >= start; i--) {
    tmpBuffer[i+1] = tmpBuffer[i];
  }
}

  
void inputLeft() {
  decPtr();
  cursorCnt = 20;
  resetPrefixSearch();
}

void inputRight() {
  incPtr();
  cursorCnt = 20;
  resetPrefixSearch();
}


void handleOskInput(u16 state, u16 diff) {
  
  if(diff & BUTTON_Y) { 
    clearOsk();
    displayingOsk = 0;
    return;
  }
  
  if(diff & BUTTON_A) { 
    // place selected key in buffer
    // increment cursor pointer
    u8 ascii_char = CHAR_START+(oskY*19)+oskX;
    
    tmpBuffer[ptr] = ascii_char;
    incPtr();
    
    resetPrefixSearch();
  }

  if(diff & BUTTON_B) {
    // delete character in buffer[ptr-1]
    // shift rest of characters left
    shiftBack(ptr-1);
    
    char delChar = tmpBuffer[ptr-1];
    if(delChar == ':') {
      defMode = 0;
    } else if (delChar == ';') {
      defMode = 1;
    }
    decPtr();
    resetPrefixSearch();
  }
  
  if(diff & BUTTON_C) {
    // shift characters starting at buffer[ptr] right
    // insert space in buffer[ptr]
    shiftForward(ptr);
    tmpBuffer[ptr] = ' ';
    incPtr();
    resetPrefixSearch();
  }
  
  // wrap-around
  if(diff & BUTTON_UP) {
    oskY = (oskY-1) % 5;
    if(oskY < 0) {
      oskY = 4;
      }
  }

  
  if(diff & BUTTON_DOWN) {
    oskY = (oskY+1);
    if(oskY >= 5) {
      oskY = 0;
    }
  }
  
  if(diff & BUTTON_LEFT) {
    if(state & BUTTON_Z) {
      inputLeft();
    } else {
      oskX = (oskX-1);
      if(oskX < 0) {
        oskX = 18;
      }
    }
  }

  if(diff & BUTTON_RIGHT) {
    if(state & BUTTON_Z) {
      inputRight();
    } else {
      oskX = (oskX+1) % 19; 
    }
  }
  
  
  
  
}

 


static int initialized=0;
static int used = 0;


// when on-screen keyboard is running
// d-pad selects keys, and doesn't move the cursor
//
void get_line_of_input() {
  
  if(initialized == 1) {
    printStrn("ok.", 3);
  } else {
    initialized = 1;
  }

  defMode = 0;
  incPrintRow();

  // reset buffer
  for(int i = 0; i < WIDTH_TILES-1; i++) {
    buffer[i] = ' ';
  }
  
  //dummy data
    /* if(used == 0) { */

    /*   buffer[0] = ':'; */
    /*   buffer[1] = ' '; */
    /*   buffer[2] = 'P'; */
    /*   buffer[3] = 'B'; */
    /*   buffer[4] = 'A'; */
    /*   buffer[5] = 'S'; */
    /*   buffer[6] = 'E'; */
    /*   buffer[7] = ' '; */
    /*   buffer[8] = 'B'; */
    /*   buffer[9] = 'A'; */
    /*   buffer[10] = 'S'; */
    /*   buffer[11] = 'E'; */
    /*   buffer[12] = ' '; */
    /*   buffer[13] = '@'; */
    /*   buffer[14] = ' '; */
    /*   buffer[15] = '.'; */
    /*   buffer[16] = ' '; */
    /*   buffer[17] = ';'; */
  
  
  
    /*   bufftop = (u32*)(buffer+WIDTH_TILES); */
    /*   curkey  = (u32*)(buffer); */
    /*   used = 1; */
    /*   return; */
    /* } */

  //char tmpBuffer[WIDTH_TILES+1];
  for(int i = 0; i < WIDTH_TILES; i++) {
    tmpBuffer[i] = ' ';
  }

  tmpBuffer[WIDTH_TILES] = '\0';
  
  ptr = minPtr;


  u16 lastState = JOY_readJoypad(JOY_1);
  
  while(1) {
    VDP_waitVSync();
    u16 state = JOY_readJoypad(JOY_1);
    
    u16 diff = (state ^ lastState) & state;


    // needs to come first to intercept other keypresses
    if(displayingOsk == 1) {
      // while z is held down && osk is displayed
      // select key from keyboard
      
      // pass input buffer and cursor pointer, along with gamepad info
      drawOskCursor(1);
      handleOskInput(state, diff);
      drawOskCursor(0);
      
      // skip rest of loop iteration
      goto end_loop_iter;
    }
 
    
    if(diff & BUTTON_LEFT) {
      inputLeft();      
    } else if (diff & BUTTON_RIGHT) {
      inputRight();
    }

    else if (diff & BUTTON_UP) {
      //read = 1;
      tmpBuffer[ptr]++;
      cursorCnt = 20;    
      resetPrefixSearch();
    }

    else if (diff & BUTTON_DOWN) {
      //read = 1;
      tmpBuffer[ptr]--;
      cursorCnt = 20;
      resetPrefixSearch();
    }
    
    if (diff & BUTTON_MODE) {
      switch (defMode) {
      case 0:
        shiftForward(ptr);
        tmpBuffer[ptr++] = ':';
        defMode++;
        break;
      case 1:
        tmpBuffer[ptr++] = ';';
        defMode++;
        break;
      }
    }
    
    if (diff & BUTTON_START) {
      resetPrefixSearch();
      break;
    }
    
    if(diff & BUTTON_A) {
      //read = 1;
      tmpBuffer[ptr] = 'A'; 
      cursorCnt = 20;     
      resetPrefixSearch();
    }
    
    if(diff & BUTTON_B) {
      //read = 1;
      shiftBack(ptr-1);

      char delChar = tmpBuffer[ptr-1];
      if(delChar == ':') {
        defMode = 0;
      } else if (delChar == ';') {
        defMode = 1;
      }
      decPtr();
      resetPrefixSearch();
    }

    if(diff & BUTTON_C) {
      //read = 1;
      shiftForward(ptr);
      tmpBuffer[ptr] = ' ';
      incPtr();
      resetPrefixSearch();
    }

    if(diff & BUTTON_X) {  
      int cnt = 0;
      
      if(tmpBuffer[ptr] == ' ') {
        continue;
      }
      
    
      // skip spaces
      while(1) {
        if(tmpBuffer[ptr+cnt] == ' ') {
          break;
        }
        cnt++;
      }
      if(prefix_cnt == 0) {
        resetPrefixSearch();
        prefix_cnt = cnt;
      }
      u32* res = prefixMatch(tmpBuffer+ptr, prefix_cnt);
      if(res) {
        u8 entryWordLen = *(((u8*)res)+4);
        char* entryWord = ((u8*)res)+5;
        if(entryWordLen > clear_cnt) {
          clear_cnt = entryWordLen;
        }
        for(int i = 0; i < clear_cnt; i++) {
          tmpBuffer[ptr+i] = ' ';
        }
        for(int i = 0; i < entryWordLen; i++) {
          //shiftForward(ptr+i+1);
          tmpBuffer[ptr+i] = entryWord[i];
        }
      }
    }

    if(diff & BUTTON_Y) {
      

      // toggle on-screen keyboard
      if(displayingOsk == 1) {
        // control should never reach this code
        clearOsk();
        displayingOsk = 0;
      } else {
        drawOsk();
        displayingOsk = 1;
      }
    }
   
  end_loop_iter:
    VDP_clearTextLine(printRow+1);
    VDP_drawTextBG(inputPlan, &tmpBuffer[0], inputFlags, 1, printRow);
    
    if(printCursor()) {
      VDP_drawTextBG(inputPlan, "-", inputFlags, ptr+1, printRow+1);
    }
    
    lastState = state;
  }
  
  VDP_clearTextLine(printRow+1);
  incPrintRow();

  // skip the null at the end
  for(int i = minPtr; i < WIDTH_TILES; i++) {
    buffer[i-minPtr+1] = charUpcase(tmpBuffer[i]);
  }
  
  
  bufftop = (u32*)(buffer+WIDTH_TILES-1);
  curkey  = (u32*)(buffer);
  
}


void printChar(char c) {

  if (c == '\n') {
    incPrintRow();
  } else {
    char str[2];
    str[0] = c;
    str[1] = '\0';
    
    VDP_drawTextBG(inputPlan, str, inputFlags, printCol, printRow);
    
    incPrintCol();
  }
}

void printStrn(char* str, u8 strLen) {
  for(int i = 0; i < strLen; i++) {
    printChar(str[i]);
  }
}

void printStrNewline(char* str, u8 strLen) {
  // if string will wrap-around, print newline first
  if(printCol+strLen >= WIDTH_TILES+1) {
    incPrintRow();
  }
  printStrn(str, strLen);
}
