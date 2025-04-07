// Author: Jadon Meredith
// Date: 4/7/2025
// Description: Test code for the LCD screen

#include <U8g2lib.h>

#define LEFT_BUTTON_PIN 53
#define RIGHT_BUTTON_PIN 51
#define ROTATE_BUTTON_PIN 49
#define DROP_BUTTON_PIN 47

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

void setup() {
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(ROTATE_BUTTON_PIN, INPUT);
  pinMode(DROP_BUTTON_PIN, INPUT);
  u8g2.begin();
  Serial.begin(9600);
  randomSeed(analogRead(0));
  block = BLOCKS[random(0, 7)];
  block = insertBlock(grid, block);
}

void loop() {
  u8g2.firstPage();
  do {
    // Set up borders
    u8g2.drawBox(0, 0, 2, 128);
    u8g2.drawBox(0, 0, 64, 4);
    u8g2.drawBox(62, 0, 2, 128);
    u8g2.drawBox(0, 124, 64, 4);

    // Draw out every tetris block currently in grid
    for (int i = 0; i < 20; i++) {
      for (int j = 0; j < 10; j++) {
        if (grid[i][j] == true) {
          u8g2.drawBox(j*BLOCK_SIZE + 2, i*BLOCK_SIZE + 4, BLOCK_SIZE, BLOCK_SIZE);
        }
      }
    }

    if (gameOver == true) {
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

  if (gameOver == false) {
    // Controls
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
      score += getScore(rowsCleared);
      block = BLOCKS[random(0, 7)];
      block = insertBlock(grid, block);
      blockMoves = 0;
    }
  }

  delay(300);
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
