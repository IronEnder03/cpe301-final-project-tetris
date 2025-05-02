// Authors: Reina Lee, Jadon Meredith, Cesar Nunez, Allison Phillips
// Date: 5/9/2025
// Description: Final project code for CPE 301

#include <U8g2lib.h>

// Pins
#define LEFT_BUTTON_PIN 53
#define RIGHT_BUTTON_PIN 51
#define ROTATE_BUTTON_PIN 49
#define DROP_BUTTON_PIN 47

#define PAUSE_BUTTON_PIN    0x02 //2
#define UNPAUSE_BUTTON_PIN  0x03 //Pin 3
#define RESET_BUTTON_PIN    0x13 //Pin 19


#define PAUSE_BUTTON_BIT    (1 << 2)
#define UNPAUSE_BUTTON_BIT  (1 << 3) 
#define RESET_BUTTON_BIT    (1 << 2) 


volatile bool isPaused = false;
volatile bool pauseRequested = false;
volatile bool unpauseRequested = false;
volatile bool resetRequested = false;
unsigned int darknessThreshold = 200;

// Registers

// Digital Pin Registers
volatile unsigned char *portDDRD = (unsigned char *) 0x2A;
volatile unsigned char *portD    = (unsigned char *) 0x2B;
volatile unsigned char *pinD     = (unsigned char *) 0x29;

volatile unsigned char *portDDRE = (unsigned char *) 0x2D;
volatile unsigned char *portE    = (unsigned char *) 0x2E;
volatile unsigned char *pinE     = (unsigned char *) 0x2C;

volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB    = (unsigned char *) 0x25;
volatile unsigned char *pinB     = (unsigned char *) 0x23;

// Timer Registers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned int *) 0x84;
volatile unsigned char *myTIFR1  = (unsigned char *) 0x36;

// UART Registers
#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
//ADC Registers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R3, 13, 11, 10, 8);

typedef struct Matrix {
    float m0, m3, m6;
    float m1, m4, m7;
    float m2, m5, m8;
} Matrix;

typedef struct Vector {
    float x;
    float y;
    float w;
} Vector;

typedef struct TetrisBlock {
    Vector b0;
    Vector b1;
    Vector b2;
    Vector b3;
    Vector origin;
} TetrisBlock;

const int BLOCK_SIZE = 6;
const TetrisBlock TETRIS_BLOCK1 = {(Vector){0, 0, 1}, (Vector){0, 1, 1}, (Vector){0, 2, 1}, (Vector){0, 3, 1}, (Vector){0.5, 1.5, 1}};
const TetrisBlock TETRIS_BLOCK2 = {(Vector){0, 1, 1}, (Vector){1, 1, 1}, (Vector){2, 1, 1}, (Vector){1, 0, 1}, (Vector){1, 0, 1}};
const TetrisBlock TETRIS_BLOCK3 = {(Vector){0, 1, 1}, (Vector){1, 1, 1}, (Vector){0, 0, 1}, (Vector){1, 0, 1}, (Vector){0.5, 0.5, 1}};
const TetrisBlock TETRIS_BLOCK4 = {(Vector){0, 0, 1}, (Vector){0, 1, 1}, (Vector){0, 2, 1}, (Vector){1, 0, 1}, (Vector){0, 1, 1}};
const TetrisBlock TETRIS_BLOCK5 = {(Vector){0, 0, 1}, (Vector){1, 0, 1}, (Vector){1, 1, 1}, (Vector){1, 2, 1}, (Vector){1, 1, 1}};
const TetrisBlock TETRIS_BLOCK6 = {(Vector){1, 0, 1}, (Vector){2, 0, 1}, (Vector){0, 1, 1}, (Vector){1, 1, 1}, (Vector){1, 1, 1}};
const TetrisBlock TETRIS_BLOCK7 = {(Vector){0, 0, 1}, (Vector){1, 0, 1}, (Vector){1, 1, 1}, (Vector){2, 1, 1}, (Vector){1, 1, 1}};

// LCD functions
Vector transformVector(Matrix mat, Vector vec);
Vector rotateVectorAroundPoint(Vector vec, Vector point, double angle);
Matrix createTransformationMatrix(float xTranslation, float yTranslation, double angle, int scale);
Matrix matrixMultiply(Matrix mat1, Matrix mat2);
bool vectorsAreEqual(Vector vector1, Vector vector2);
TetrisBlock translateBlock(bool grid[20][10], TetrisBlock block, int xTranslation, int yTranslation);
TetrisBlock rotateBlock(bool grid[20][10], TetrisBlock block, double angle);
TetrisBlock insertBlock(bool grid[20][10], TetrisBlock block);
TetrisBlock dropBlock(bool brid[20][10], TetrisBlock block);
bool canMoveDown(bool grid[20][10], TetrisBlock block);
bool canMoveLeft(bool grid[20][10], TetrisBlock block);
bool canMoveRight(bool grid[20][10], TetrisBlock block);
bool canRotate(bool grid[20][10], TetrisBlock block);
void clearGrid(bool grid[20][10]);
int clearRows(bool grid[20][10], TetrisBlock block);
int getScore(int rowsCleared);

// Delay and buzzer functions
void myDelay(unsigned int freq);
void playTetrisTheme();

// UART functions
void U0init(int U0baud);
unsigned char U0kbhit();
unsigned char U0getchar();
void U0putchar(unsigned char U0pdata);

// ADC functions
void adc_init();
unsigned int adc_read(unsigned char adc_channel_num);

const TetrisBlock BLOCKS[7] = {TETRIS_BLOCK1, TETRIS_BLOCK2, TETRIS_BLOCK3, TETRIS_BLOCK4, TETRIS_BLOCK5, TETRIS_BLOCK6, TETRIS_BLOCK7};

bool grid[20][10] = {{0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0},
                     {0,0,0,0,0,0,0,0,0,0}};

bool gameOver = false;
TetrisBlock block;
int blockMoves = 0;
int score = 0;
unsigned int freq = 0;
unsigned long previousMillis = millis();
unsigned int currentNote = 0;
unsigned int gameDelay = 600;
unsigned int totalRowsCleared = 0;
unsigned int level = 1;

void setup() {
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(ROTATE_BUTTON_PIN, INPUT);
  pinMode(DROP_BUTTON_PIN, INPUT);

  *portDDRB |= (1 << 6);  // Same as 0x40

  u8g2.begin();
  U0init(9600);
  adc_init();
  randomSeed(adc_read(0));
  
  block = BLOCKS[random(0, 7)];
  block = insertBlock(grid, block);

  *portDDRD &= ~(PAUSE_BUTTON_BIT | UNPAUSE_BUTTON_BIT);
  *portDDRE &= ~(RESET_BUTTON_BIT);                 

  *portD |= (PAUSE_BUTTON_BIT | UNPAUSE_BUTTON_BIT);
  *portE |= (RESET_BUTTON_BIT);

  attachInterrupt(digitalPinToInterrupt(PAUSE_BUTTON_PIN), pauseISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(UNPAUSE_BUTTON_PIN), unpauseISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), resetISR, FALLING);
}

bool pausedByDark = false;

void loop() {

  unsigned int sensor_value = adc_read(8);
  Serial.println(sensor_value);
  bool isDark = (sensor_value < darknessThreshold);

  if (isDark && !isPaused) 
  {
    isPaused = true;
    pausedByDark = true;
  }else if(!isDark && pausedByDark) 
  {
    isPaused = false;
    pausedByDark = false;
  }
  if (pauseRequested) 
  {
    isPaused = true;
    pauseRequested = false;
    pausedByDark = false;
  }

  if (unpauseRequested) 
  {
    isPaused = false;
    unpauseRequested = false;
    pausedByDark = false;
  }
  if (resetRequested) 
  {
  resetRequested = false;
  pausedByDark = false;
  resetGame();
  }

  unsigned long currentMillis = millis();
  if (!isPaused && currentMillis - previousMillis > (gameDelay / level)) {

    if (digitalRead(LEFT_BUTTON_PIN) == HIGH && canMoveLeft(grid, block)) {
      block = translateBlock(grid, block, -1, 0);
    }
    else if (digitalRead(RIGHT_BUTTON_PIN) == HIGH && canMoveRight(grid, block)) {
      block = translateBlock(grid, block, 1, 0);
    }
    else if (digitalRead(ROTATE_BUTTON_PIN) == HIGH && canRotate(grid, block)) {
      block = rotateBlock(grid, block);
    }
    else if (digitalRead(DROP_BUTTON_PIN) == HIGH) {
      block = dropBlock(grid, block);
      blockMoves++;
    }

    // Move block down and check game over and if new block is needed
    if (canMoveDown(grid, block)) {
      block = translateBlock(grid, block, 0, 1);
      blockMoves++;
    } else if (blockMoves == 0) {
      gameOver = true;
    } else {
      int rowsCleared = clearRows(grid, block);
      totalRowsCleared += rowsCleared;
      if (rowsCleared != 0 && totalRowsCleared % 2 == 0) {
        level += 1;
      }
      score += getScore(rowsCleared);
      block = BLOCKS[random(0, 7)];
      block = insertBlock(grid, block);
      blockMoves = 0;
    }

    previousMillis = currentMillis;
  }

  //
  u8g2.firstPage();
  do {
    if (isPaused) {
      u8g2.setFont(u8g2_font_bpixel_tr);
      u8g2.drawStr(20, 30, "PAUSED");
    } else {
      // Draw borders
      u8g2.drawBox(0, 0, 2, 128);
      u8g2.drawBox(0, 0, 64, 4);
      u8g2.drawBox(62, 0, 2, 128);
      u8g2.drawBox(0, 124, 64, 4);

      // Draw blocks
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
          if (grid[i][j]) {
            u8g2.drawBox(j * BLOCK_SIZE + 2, i * BLOCK_SIZE + 4, BLOCK_SIZE, BLOCK_SIZE);
          }
        }
      }
    }

    if (gameOver) {
      clearGrid(grid);
      u8g2.setFont(u8g2_font_bpixel_tr);
      u8g2.drawStr(5, 30, "Game Over!");
      u8g2.setFont(u8g2_font_squeezed_b6_tr);
      char scoreStr[10];
      itoa(score, scoreStr, 10);
      u8g2.drawStr(5, 40, "Score:");
      u8g2.drawStr(38, 40, scoreStr);
    }
  } while (u8g2.nextPage());

  if (!isPaused && !gameOver) {
    playTetrisTheme();
  }
}

Vector transformVector(Matrix mat, Vector vec) {
    Vector newVec;
    newVec.x = (mat.m0 * vec.x) + (mat.m3 * vec.y) + (mat.m6 * vec.w);
    newVec.y = (mat.m1 * vec.x) + (mat.m4 * vec.y) + (mat.m7 * vec.w);
    newVec.w = (mat.m2 * vec.x) + (mat.m5 * vec.y) + (mat.m8 * vec.w);
    return newVec;
}

Vector rotateVectorAroundPoint(Vector vec, Vector point, double angle) {
    Matrix mat1 = createTransformationMatrix(point.x, point.y, 0, 1);
    Matrix mat2 = createTransformationMatrix(0, 0, angle, 1);
    Matrix mat3 = createTransformationMatrix(-point.x, -point.y, 0, 1);
    Vector rotatedVector;
    rotatedVector = transformVector(mat3, vec);
    rotatedVector = transformVector(mat2, rotatedVector);
    rotatedVector = transformVector(mat1, rotatedVector);
    return rotatedVector;
}

Matrix createTransformationMatrix(float xTranslation, float yTranslation, double angle, int scale) {
    Matrix mat;
    double radians = angle * M_PI / 180.0;
    mat.m0 = cos(radians) * scale;
    mat.m1 = sin(radians);
    mat.m2 = 0;
    mat.m3 = -sin(radians);
    mat.m4 = cos(radians) * scale;
    mat.m5 = 0;
    mat.m6 = xTranslation;
    mat.m7 = yTranslation;
    mat.m8 = 1;
    return mat;
}

Matrix matrixMultiply(Matrix mat1, Matrix mat2) {
    Matrix newMat;
    newMat.m0 = (mat1.m0 * mat2.m0) + (mat1.m3 * mat2.m1) + (mat1.m6 * mat2.m2);
    newMat.m1 = (mat1.m1 * mat2.m0) + (mat1.m4 * mat2.m1) + (mat1.m7 * mat2.m2);
    newMat.m2 = (mat1.m2 * mat2.m0) + (mat1.m5 * mat2.m1) + (mat1.m8 * mat2.m2);
    newMat.m3 = (mat1.m0 * mat2.m3) + (mat1.m3 * mat2.m4) + (mat1.m6 * mat2.m5);
    newMat.m4 = (mat1.m1 * mat2.m3) + (mat1.m4 * mat2.m4) + (mat1.m7 * mat2.m5);
    newMat.m5 = (mat1.m2 * mat2.m3) + (mat1.m5 * mat2.m4) + (mat1.m8 * mat2.m5);
    newMat.m6 = (mat1.m0 * mat2.m6) + (mat1.m3 * mat2.m7) + (mat1.m6 * mat2.m8);
    newMat.m7 = (mat1.m1 * mat2.m6) + (mat1.m4 * mat2.m7) + (mat1.m7 * mat2.m8);
    newMat.m8 = (mat1.m2 * mat2.m6) + (mat1.m5 * mat2.m7) + (mat1.m8 * mat2.m8);
    return newMat;
}

bool vectorsAreEqual(Vector vector1, Vector vector2) {
  if (round(vector1.x) == round(vector2.x) && round(vector1.y) == round(vector2.y) && round(vector1.w) == round(vector2.w)) {
    return true;
  } else {
    return false;
  }
}

TetrisBlock translateBlock(bool grid[20][10], TetrisBlock block, int xTranslation, int yTranslation) {
    Matrix translationMat = createTransformationMatrix(xTranslation, yTranslation, 0, 1);
    grid[round(block.b0.y)][round(block.b0.x)] = false;
    grid[round(block.b1.y)][round(block.b1.x)] = false;
    grid[round(block.b2.y)][round(block.b2.x)] = false;
    grid[round(block.b3.y)][round(block.b3.x)] = false;
    block.b0 = transformVector(translationMat, block.b0);
    block.b1 = transformVector(translationMat, block.b1);
    block.b2 = transformVector(translationMat, block.b2);
    block.b3 = transformVector(translationMat, block.b3);
    block.origin = transformVector(translationMat, block.origin);
    grid[round(block.b0.y)][round(block.b0.x)] = true;
    grid[round(block.b1.y)][round(block.b1.x)] = true;
    grid[round(block.b2.y)][round(block.b2.x)] = true;
    grid[round(block.b3.y)][round(block.b3.x)] = true;
    return block;
}

TetrisBlock rotateBlock(bool grid[20][10], TetrisBlock block) {
    grid[round(block.b0.y)][round(block.b0.x)] = false;
    grid[round(block.b1.y)][round(block.b1.x)] = false;
    grid[round(block.b2.y)][round(block.b2.x)] = false;
    grid[round(block.b3.y)][round(block.b3.x)] = false;

    block.b0 = rotateVectorAroundPoint(block.b0, block.origin, 90);
    block.b1 = rotateVectorAroundPoint(block.b1, block.origin, 90);
    block.b2 = rotateVectorAroundPoint(block.b2, block.origin, 90);
    block.b3 = rotateVectorAroundPoint(block.b3, block.origin, 90);

    while (block.b0.x < 0 || block.b1.x < 0 || block.b2.x < 0 || block.b3.x < 0) {
      block.b0.x++;
      block.b1.x++;
      block.b2.x++;
      block.b3.x++;
      block.origin.x++;
    }

    while (block.b0.x > 9 || block.b1.x > 9 || block.b2.x > 9 || block.b3.x > 9) {
      block.b0.x--;
      block.b1.x--;
      block.b2.x--;
      block.b3.x--;
      block.origin.x--;
    }

    grid[round(block.b0.y)][round(block.b0.x)] = true;
    grid[round(block.b1.y)][round(block.b1.x)] = true;
    grid[round(block.b2.y)][round(block.b2.x)] = true;
    grid[round(block.b3.y)][round(block.b3.x)] = true;

    return block;
}

TetrisBlock insertBlock(bool grid[20][10], TetrisBlock block) {
  block = translateBlock(grid, block, 4, 0);
  return block;
}

TetrisBlock dropBlock(bool brid[20][10], TetrisBlock block) {
  while (canMoveDown(grid, block)) {
    block = translateBlock(grid, block, 0, 1);
  }
  return block;
}

bool canMoveDown(bool grid[20][10], TetrisBlock block) {
  TetrisBlock translatedBlock = block;
  translatedBlock.b0.y++;
  translatedBlock.b1.y++;
  translatedBlock.b2.y++;
  translatedBlock.b3.y++;
  translatedBlock.origin.y++;
  if (block.b0.y < 19 && block.b1.y < 19 && block.b2.y < 19 && block.b3.y < 19) {
    if ((grid[round(translatedBlock.b0.y)][round(translatedBlock.b0.x)] == 0 || (vectorsAreEqual(translatedBlock.b0, block.b0) || vectorsAreEqual(translatedBlock.b0, block.b1) || vectorsAreEqual(translatedBlock.b0, block.b2) || vectorsAreEqual(translatedBlock.b0, block.b3)))
    && (grid[round(translatedBlock.b1.y)][round(translatedBlock.b1.x)] == 0 || (vectorsAreEqual(translatedBlock.b1, block.b0) || vectorsAreEqual(translatedBlock.b1, block.b1) || vectorsAreEqual(translatedBlock.b1, block.b2) || vectorsAreEqual(translatedBlock.b1, block.b3))) 
    && (grid[round(translatedBlock.b2.y)][round(translatedBlock.b2.x)] == 0 || (vectorsAreEqual(translatedBlock.b2, block.b0) || vectorsAreEqual(translatedBlock.b2, block.b1) || vectorsAreEqual(translatedBlock.b2, block.b2) || vectorsAreEqual(translatedBlock.b2, block.b3))) 
    && (grid[round(translatedBlock.b3.y)][round(translatedBlock.b3.x)] == 0 || (vectorsAreEqual(translatedBlock.b3, block.b0) || vectorsAreEqual(translatedBlock.b3, block.b1) || vectorsAreEqual(translatedBlock.b3, block.b2) || vectorsAreEqual(translatedBlock.b3, block.b3)))) {
      return true;
    }
  }
  return false;
}

bool canMoveLeft(bool grid[20][10], TetrisBlock block) {
  if (block.b0.x > 0 && block.b1.x > 0 && block.b2.x > 0 && block.b3.x > 0) {
    if ((grid[round(block.b0.y)][round(block.b0.x-1)] == 0 || (block.b0.x-1 == block.b1.x || block.b0.x-1 == block.b2.x || block.b0.x-1 == block.b3.x))
    && (grid[round(block.b1.y)][round(block.b1.x-1)] == 0 || (block.b1.x-1 == block.b0.x || block.b1.x-1 == block.b2.x || block.b1.x-1 == block.b3.x)) 
    && (grid[round(block.b2.y)][round(block.b2.x-1)] == 0 || (block.b2.x-1 == block.b0.x || block.b2.x-1 == block.b1.x || block.b2.x-1 == block.b3.x)) 
    && (grid[round(block.b3.y)][round(block.b3.x-1)] == 0 || (block.b3.x-1 == block.b0.x || block.b3.x-1 == block.b1.x || block.b3.x-1 == block.b2.x))) {
      return true;
    }
  }
  return false;
}

bool canMoveRight(bool grid[20][10], TetrisBlock block) {
  if (block.b0.x < 9 && block.b1.x < 9 && block.b2.x < 9 && block.b3.x < 9) {
    if ((grid[round(block.b0.y)][round(block.b0.x+1)] == 0 || (block.b0.x+1 == block.b1.x || block.b0.x+1 == block.b2.x || block.b0.x+1 == block.b3.x))
    && (grid[round(block.b1.y)][round(block.b1.x+1)] == 0 || (block.b1.x+1 == block.b0.x || block.b1.x+1 == block.b2.x || block.b1.x+1 == block.b3.x)) 
    && (grid[round(block.b2.y)][round(block.b2.x+1)] == 0 || (block.b2.x+1 == block.b0.x || block.b2.x+1 == block.b1.x || block.b2.x+1 == block.b3.x)) 
    && (grid[round(block.b3.y)][round(block.b3.x+1)] == 0 || (block.b3.x+1 == block.b0.x || block.b3.x+1 == block.b1.x || block.b3.x+1 == block.b2.x))) {
      return true;
    }
  }
  return false;
}

bool canRotate(bool grid[20][10], TetrisBlock block) {
  TetrisBlock rotatedBlock = block;
  rotatedBlock.b0 = rotateVectorAroundPoint(block.b0, block.origin, 90);
  rotatedBlock.b1 = rotateVectorAroundPoint(block.b1, block.origin, 90);
  rotatedBlock.b2 = rotateVectorAroundPoint(block.b2, block.origin, 90);
  rotatedBlock.b3 = rotateVectorAroundPoint(block.b3, block.origin, 90);

  while (block.b0.x < 0 || block.b1.x < 0 || block.b2.x < 0 || block.b3.x < 0) {
      rotatedBlock.b0.x++;
      rotatedBlock.b1.x++;
      rotatedBlock.b2.x++;
      rotatedBlock.b3.x++;
      rotatedBlock.origin.x++;
    }

    while (block.b0.x > 9 || block.b1.x > 9 || block.b2.x > 9 || block.b3.x > 9) {
      rotatedBlock.b0.x--;
      rotatedBlock.b1.x--;
      rotatedBlock.b2.x--;
      rotatedBlock.b3.x--;
      rotatedBlock.origin.x--;
    }

  if ((vectorsAreEqual(rotatedBlock.b0, block.b0) || vectorsAreEqual(rotatedBlock.b0, block.b1) || vectorsAreEqual(rotatedBlock.b0, block.b2) || vectorsAreEqual(rotatedBlock.b0, block.b3) || grid[round(rotatedBlock.b0.y)][round(rotatedBlock.b0.x)] == 0) 
  && (vectorsAreEqual(rotatedBlock.b1, block.b0) || vectorsAreEqual(rotatedBlock.b1, block.b1) || vectorsAreEqual(rotatedBlock.b1, block.b2) || vectorsAreEqual(rotatedBlock.b1, block.b3) || grid[round(rotatedBlock.b1.y)][round(rotatedBlock.b1.x)] == 0)
  && (vectorsAreEqual(rotatedBlock.b2, block.b0) || vectorsAreEqual(rotatedBlock.b2, block.b1) || vectorsAreEqual(rotatedBlock.b2, block.b2) || vectorsAreEqual(rotatedBlock.b2, block.b3) || grid[round(rotatedBlock.b2.y)][round(rotatedBlock.b2.x)] == 0) 
  && (vectorsAreEqual(rotatedBlock.b3, block.b0) || vectorsAreEqual(rotatedBlock.b3, block.b1) || vectorsAreEqual(rotatedBlock.b3, block.b2) || vectorsAreEqual(rotatedBlock.b3, block.b3) || grid[round(rotatedBlock.b3.y)][round(rotatedBlock.b3.x)] == 0)) {
    return true;
  } else {
    return false;
  }
}

void clearGrid(bool grid[20][10]) {
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 10; j++) {
      grid[i][j] = 0;
    }
  }
}

int clearRows(bool grid[20][10], TetrisBlock block) {
  int rowsCleared = 0;
  int minY = min(min(block.b0.y, block.b1.y), min(block.b2.y, block.b3.y));

  // Checks if there is a complete row
  for (int i = minY; i <= minY+3; i++) {
    int blocksPresent = 0;
    for (int j = 0; j < 10; j++) {
      if (grid[i][j] == 1) {
        blocksPresent++;
      }
    }

    // Clears row if it is a complete row
    if (blocksPresent == 10) {
      for (int j = 0; j < 10; j++) {
        grid[i][j] = 0;
      }
      // Shifts every other block down by one row
      for (int j = i; j > 0; j--) {
        for (int k = 0; k < 10; k++) {
          grid[j][k] = grid[j-1][k];
        }
      }
      rowsCleared++;
      if (rowsCleared == 4) {
        return rowsCleared;
      }
    }
  }
  return rowsCleared;
}

int getScore(int rowsCleared) {
  if (rowsCleared == 1) {
    return 100;
  } else if (rowsCleared == 2) {
    return 300;
  } else if (rowsCleared == 3) {
    return 500;
  } else if (rowsCleared == 4) {
    return 800;
  } else {
    return 0;
  }
}

// -------------------------------------------------------
// myDelay: Delays for half‑period based on freq
// using Timer1 in normal mode (blocking).
// -------------------------------------------------------
void myDelay(unsigned int freq) {
  if (freq == 0) return; // do nothing if freq=0

  // Calculate half‑period in seconds
  double period      = 1.0 / double(freq);
  double half_period = period / 2.0;

  // CPU clock period = 1/(16MHz) = 62.5 ns = 0.0000000625 s
  double clk_period  = 0.0000000625;

  // How many timer counts for half‑period
  unsigned int ticks = (unsigned int)(half_period / clk_period);

  // (Re)configure Timer1
  *myTCCR1A = 0x0;

  // Stop the timer
  *myTCCR1B &= 0xF8;         // 0b1111 1000
  // Preload so overflow hits at half‑period
  *myTCNT1  = (unsigned int)(65536 - ticks);

  // Start the timer (no prescaler)
  *myTCCR1B |= 0x01;         // 0b0000 0001

  // Wait for overflow flag
  while ((*myTIFR1 & 0x01) == 0) {
    // blocking wait
  }

  // Stop the timer
  *myTCCR1B &= 0xF8;         // 0b1111 1000
  // Clear TOV1 overflow flag
  *myTIFR1  |= 0x01;         // 0b0000 0001
}

// -------------------------------------------------------
// playTetrisTheme: Loops a short Tetris snippet
// indefinitely until 'q' is typed.
// -------------------------------------------------------
void playTetrisTheme() {
  // Named duration variables
  int qu  = 300 / level;  // "quarter" note in ms
  int eth = 150 / level;  // "eighth" note in ms
  int hn = 600 / level; // "halfnote" note in ms

  // Example snippet of Tetris (Korobeiniki) theme
  // Feel free to expand the arrays for a longer melody
  unsigned int melody[] = {
    // measure 1
    659, 494, 523, 587,   // E5, B4, C5, D5
    659, 523, 494,   // E5, C5, B4,
    // measure 2
    440, 440, 523, 659, 587, 523,//A4, A4, C5, E5, D5, C5
    //measure 3
    494, 494, 523, 587, 659,//B4, B4, C5, D5, E5
    //measure 4
    523, 440, 440,//C5, A4, A4,
    //mesure 5
   0, 587, 698, 880, 784, 698, //REST, D5, F5, A5, G5, F5
    //measure 6
    0, 659, 523, 659, 587, 523,//REST, E5, C5, E5, D5, C5
    //measure 7
    494, 494, 523, 587, 659,//B4, B4, C5, D5, E5
    //measure 8
    523, 440, 440,//C5, A4, A4 REPEAT MEASURES 1-8
    // measure 1
    659, 494, 523, 587,   // E5, B4, C5, D5
    659, 523, 494,   // E5, C5, B4,
    // measure 2
    440, 440, 523, 659, 587, 523,//A4, A4, C5, E5, D5, C5
    //measure 3
    494, 494, 523, 587, 659,//B4, B4, C5, D5, E5
    //measure 4
    523, 440, 440,//C5, A4, A4,
    //mesure 5
    0, 587, 698, 880, 784, 698, //REST, D5, F5, A5, G5, F5
    //measure 6
    0, 659, 523, 659, 587, 523,//REST, E5, C5, E5, D5, C5
    //measure 7
    494, 494, 523, 587, 659,//B4, B4, C5, D5, E5
    //measure 8
    523, 440, 440,//C5, A4, A4 REPEAT MEASURES 1-8
    //measure 9
    659, 494, 523, 587, 659,//E5, B4, C5, D5, E5
    //measure 10
    523, 440, 523, 659, 880,//C5, A4, C5, E5, A5
    //measure 11
    831, 695, 659, 587, 523,//g5, F5, E5, D5, C5
    //measure 12
    659, 622, 587, 554,//E5, d5, D5, c5
    //measure 13
    523, 440, 988, 523, 440,//C5, A4, B5, C5, A4
    //measure 14
    988, 330, 415, 494, 392,//B5, E4, g4, B4, G4
    //measure 15
    440, 392, 349, 294,//A4, G4, F4, D4
    //measure 16
    440, 392, 349, 294,//A4, G4, F4, D4
    //measure 17
    330, 311, 330, 349, 415, 440, 494, 523,//E4, d4, E4, F4, g4, A4, B4, C5
    //measure 18
    587, 523, 587, 523, 494, 440, 415, 330,//D5, C5, D5, C5, B4, A4, g4, E4
    //measure 19
    415, 440, 494, 523, //g4, A4, B4, C5,
    587, 622, 659, 831, 659// D5, d5, E5, g5, E5
  };

  // Matching durations (in milliseconds) using our named vars
  // Adjust to your liking for variety (eth or qu, etc.)
  unsigned int noteDurations[] = {
    // measure 1
    qu, eth, eth, eth,
    eth, eth, eth,  
    //measure 2
    qu, eth, eth, 
    qu, eth, eth,
    //measure 3 
    qu, eth, eth, qu, qu,
    //measure 4
    qu, qu, hn,
    //measure 5
    eth, qu, eth, 
    qu, eth, eth,
    //measure 6
    eth, qu, eth, qu, eth, eth,
    //measure 7
    qu, eth, eth, qu, qu,
    //measure 8
    qu, qu, hn, //repeat measures 1-8
    // measure 1
    qu, eth, eth, eth,
    eth, eth, eth,  
    //measure 2
    qu, eth, eth, 
    qu, eth, eth,
    //measure 3 
    qu, eth, eth, qu, qu,
    //measure 4
    qu, qu, hn,
    //measure 5
    eth, qu, eth, 
    qu, eth, eth,
    //measure 6
    eth, qu, eth, qu, eth, eth,
    //measure 7
    qu, eth, eth, qu, qu,
    //measure 8
    qu, qu, hn, 
    //measure 9 
    qu, eth, eth, qu, qu,
    //measure 10
    qu, eth, eth, qu, qu,
    //measure 11
    qu, eth, eth, qu, qu,
    //measure 12
    qu, qu, qu, qu,
    //measure 13
    qu, eth, eth, qu, qu,
    //measure 14
    qu, eth, eth, qu, qu,
    //measure 15
    qu, qu, qu, qu,
    //measure 16
    qu, qu, qu, qu, 
    //measure 17
    eth, eth, eth, eth, 
    eth, eth, eth, eth, 
    //measure 18
    eth, eth, eth, eth, 
    eth, eth, eth, eth, 
    //measure 19
    eth, eth, eth, eth,
    eth, eth, eth, eth, hn};

  // Total number of notes in the snippet
  const int numNotes = sizeof(melody) / sizeof(melody[0]);

  // Set the current note
  freq = melody[currentNote];

  // Play for noteDurations[i] milliseconds
  unsigned long startTime = millis();
  while (millis() - startTime < noteDurations[currentNote]) {
    // Toggle PB6 using half-period toggles
    *portB ^= 0x40;
    myDelay(freq);
  }
  currentNote++;
  if (currentNote == numNotes) {
    currentNote = 0;
  }
}

void U0init(int U0baud) {
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char U0kbhit() {
  return *myUCSR0A & RDA;
}

unsigned char U0getchar() {
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata) {
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void adc_init() {
  // setup the A register
 // set bit   7 to 1 to enable the ADC
 *my_ADCSRA |= 0b10000000;

 // clear bit 6 to 0 to disable the ADC trigger mode
 *my_ADCSRA &= 0b10111111;

 // clear bit 5 to 0 to disable the ADC interrupt
 *my_ADCSRA &= 0b11011111;

 // clear bit 0-2 to 0 to set prescaler selection to slow reading
 *my_ADCSRA &= 0b11111000;

  // setup the B register
// clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11110111;

 // clear bit 2-0 to 0 to set free running mode
 *my_ADCSRB &= 0b11111000;

  // setup the MUX Register
  // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX &= 0b0000000;

  // set bit 6 to 1 for AVCC analog reference
  *my_ADMUX |= 0b01000000;

  // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11011111;

  // clear bit 4-0 to 0 to reset the channel and gain bits
  *my_ADMUX &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num) {
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;

  // clear the channel selection bits (MUX 5) hint: it's not in the ADMUX register
  *my_ADCSRB &= 0b01110111;

  if (adc_channel_num > 7)
    *my_ADCSRB |= 0b00001000;
 
  // set the channel selection bits for channel
  switch (adc_channel_num){
    case 0:
    case 8:
      *my_ADMUX |= 0b01000000;
      break;
    case 1:
    case 9:
      *my_ADMUX |= 0b01000001;
      break;
    case 2:
    case 10:
      *my_ADMUX |= 0b01000010;
      break;
    case 3:
    case 11:
      *my_ADMUX |= 0b01000011;
      break;
    case 4:
    case 12:
      *my_ADMUX |= 0b01000100;
      break;
    case 5:
    case 13:
      *my_ADMUX |= 0b01000101;
      break;
    case 6:
    case 14:
      *my_ADMUX |= 0b01000110;
      break;
    case 7:
    case 15:
      *my_ADMUX |= 0b01000111;
      break;
  }

  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0b01000000;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register and format the data based on right justification 
  unsigned int val = *my_ADC_DATA;
  return val;
}

void resetGame() 
{
  clearGrid(grid); 
  block = BLOCKS[random(0, 7)];
  block = insertBlock(grid, block);
  blockMoves = 0;
  totalRowsCleared = 0;
  level = 1;
  score = 0;
  gameOver = false;
  isPaused = false;
  previousMillis = millis(); 
}

void pauseISR() 
{
  pauseRequested = true;
}

void unpauseISR() 
{
  unpauseRequested = true;
}
void resetISR() 
{
  resetRequested = true;
}