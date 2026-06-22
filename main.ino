#include "Arduino_LED_Matrix.h"
#define MAX_LENGTH 96
#include <cmath>
ArduinoLEDMatrix matrix;

byte frame[8][12] = {0};

unsigned long lastMoveTime = 0;
const int gameSpeed = 250; // low is faster

int body[MAX_LENGTH][2]; 
const int btnUp = 3;
const int btnDown = 2;
const int btnLeft = 5;
const int btnRight = 4;

int dir = 3;     
int nextDir = 3; 
int length = 3;

int gameMode = 8;
// 0 - none
// 1 - no warp
// 2 - teleport
// 3 - inverse
// 4 - c apples
// 5 - 1d6 apples
// 6 - chese
// 7 - hot dog
// 8 - hidden apples (they pulse)

const int c = 5;
int apple[6][2]; 
int activeApples = 1;
unsigned long appleSpawnTime[6];

void refreshMatrix() {
  uint32_t renderBuffer[3] = {0, 0, 0};
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 12; x++) {
      if (frame[y][x]) {
        int bitIndex = y * 12 + x;
        int wordIndex = bitIndex / 32;
        int bitPosition = 31 - (bitIndex % 32); 
        renderBuffer[wordIndex] |= (1UL << bitPosition);
      }
    }
  }
  matrix.loadFrame(renderBuffer);
}

void writePixel(int x, int y, int state) {
  if (x >= 0 && x < 12 && y >= 0 && y < 8) {
    frame[y][x] = state;
  }
}

void clear() {
  memset(frame, 0, sizeof(frame));
}

void determineAppleCount() {
  if (gameMode == 2) activeApples = 2;
  else if (gameMode == 4) activeApples = c;
  else if (gameMode == 5) activeApples = random(1, 7);
  else activeApples = 1;
  
  if (activeApples > 6) activeApples = 6;
}

void spawnSingleApple(int a) {
  bool onBody;
  do {
    onBody = false;
    apple[a][0] = random(0, 12); 
    apple[a][1] = random(0, 8);
    
    for (int i = 0; i < length; i++) {
      if (gameMode == 6 &&  i > 0 && ((body[i][0] + body[i][1]) % 2 != 0)) continue;

      if (body[i][0] == apple[a][0] && body[i][1] == apple[a][1]) {
        onBody = true;
        break;
      }

      if (gameMode == 7 && i > 0) {
        int px = body[i][0], py = body[i][1];
        int prevX = body[i - 1][0], prevY = body[i - 1][1];
        int dx = abs(prevX - px), dy = abs(prevY - py);
        
        int b1x = px, b1y = py, b2x = px, b2y = py;
        if (dx == 1 || dx == 11) { b1y = (py + 1) % 8; b2y = (py + 7) % 8; }
        else if (dy == 1 || dy == 7) { b1x = (px + 1) % 12; b2x = (px + 11) % 12; }
        
        if ((apple[a][0] == b1x && apple[a][1] == b1y) || (apple[a][0] == b2x && apple[a][1] == b2y)) {
          onBody = true;
          break;
        }
      }
    }
    
    for (int i = 0; i < activeApples; i++) {
      if (i != a && apple[i][0] == apple[a][0] && apple[i][1] == apple[a][1]) {
        onBody = true;
        break;
      }
    }
    appleSpawnTime[a] = millis();
  } while (onBody == true);
}

void spawnAllApples() {
  for (int a = 0; a < activeApples; a++) {
    spawnSingleApple(a);
  }
  for (int a = 0; a < activeApples; a++) writePixel(apple[a][0], apple[a][1], 1);
}

void drawGame() {
  clear();
  for (int i = 0; i < length; i++) {
    if (gameMode == 6 &&  i > 0 && ((body[i][0] + body[i][1]) % 2 != 0)) continue;
    
    writePixel(body[i][0], body[i][1], 1);
    
    if (gameMode == 7 && i > 0) {
      int px = body[i][0], py = body[i][1];
      int prevX = body[i - 1][0], prevY = body[i - 1][1];
      int dx = abs(prevX - px), dy = abs(prevY - py);
      
      if (dx == 1 || dx == 11) {
        writePixel(px, (py + 1) % 8, 1);
        writePixel(px, (py + 7) % 8, 1);
      } else if (dy == 1 || dy == 7) {
        writePixel((px + 1) % 12, py, 1);
        writePixel((px + 11) % 12, py, 1);
      }
    }
  }
  if (gameMode == 8) {
    for (int a = 0; a < activeApples; a++) {
      if (millis() - appleSpawnTime[a] < 1000) { 
        writePixel(apple[a][0], apple[a][1], 1);
      }
    }
  } else {
    for (int a = 0; a < activeApples; a++) writePixel(apple[a][0], apple[a][1], 1);
  }
  refreshMatrix();
}

void resetGame() {
  length = 3;
  dir = 3;
  nextDir = 3;
  
  body[0][0] = 3; body[0][1] = 4; 
  body[1][0] = 2; body[1][1] = 4; 
  body[2][0] = 1; body[2][1] = 4; 
  
  determineAppleCount();
  spawnAllApples();
  drawGame(); 
}

void setup() {
  matrix.begin();
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  randomSeed(analogRead(0)); 
  
  resetGame(); 
}

void loop() {
  if (digitalRead(btnUp) == LOW    && dir != 1) nextDir = 0;
  if (digitalRead(btnDown) == LOW  && dir != 0) nextDir = 1;
  if (digitalRead(btnLeft) == LOW  && dir != 3) nextDir = 2;
  if (digitalRead(btnRight) == LOW && dir != 2) nextDir = 3;

  if (millis() - lastMoveTime >= gameSpeed) {
    dir = nextDir; 

    int oldTailX = body[length - 1][0];
    int oldTailY = body[length - 1][1];

    for (int i = length - 1; i > 0; i--) {
      body[i][0] = body[i - 1][0];
      body[i][1] = body[i - 1][1];
    }

    int newHeadX = body[0][0];
    int newHeadY = body[0][1];

    if (dir == 0) newHeadY--; 
    if (dir == 1) newHeadY++; 
    if (dir == 2) newHeadX--; 
    if (dir == 3) newHeadX++; 

    if (gameMode == 1) {
      if (newHeadX < 0 || newHeadX > 11 || newHeadY < 0 || newHeadY > 7) {
        delay(500);
        resetGame();
        return;
      }
    }

    body[0][0] = (newHeadX + 12) % 12;
    body[0][1] = (newHeadY + 8) % 8;

    for (int i = 1; i < length; i++) {
      if (gameMode == 6 && i > 0 && ((body[i][0] + body[i][1]) % 2 != 0)) continue;

      if (body[0][0] == body[i][0] && body[0][1] == body[i][1]) {
        delay(500);
        resetGame();
        return; 
      }

      if (gameMode == 7) {
        int px = body[i][0], py = body[i][1];
        int prevX = body[i - 1][0], prevY = body[i - 1][1];
        int dx = abs(prevX - px), dy = abs(prevY - py);
        
        int b1x = px, b1y = py, b2x = px, b2y = py;
        if (dx == 1 || dx == 11) { b1y = (py + 1) % 8; b2y = (py + 7) % 8; }
        else if (dy == 1 || dy == 7) { b1x = (px + 1) % 12; b2x = (px + 11) % 12; }
        
        if ((body[0][0] == b1x && body[0][1] == b1y) || (body[0][0] == b2x && body[0][1] == b2y)) {
          delay(500);
          resetGame();
          return;
        }
      }
    }

    int eatenApple = -1;
    for (int a = 0; a < activeApples; a++) {
      if (body[0][0] == apple[a][0] && body[0][1] == apple[a][1]) {
        eatenApple = a;
        break;
      }
    }

    if (eatenApple != -1) {
      if (gameMode == 2) {
        int targetApple = (eatenApple == 0) ? 1 : 0;
        body[0][0] = apple[targetApple][0];
        body[0][1] = apple[targetApple][1];
      }

      if (length < MAX_LENGTH) {
        body[length][0] = oldTailX;
        body[length][1] = oldTailY;
        length++; 
      }

      if (gameMode == 3) {
        for (int i = 0; i < length / 2; i++) {
          int tempX = body[i][0];
          int tempY = body[i][1];
          body[i][0] = body[length - 1 - i][0];
          body[i][1] = body[length - 1 - i][1];
          body[length - 1 - i][0] = tempX;
          body[length - 1 - i][1] = tempY;
        }
        int dx = body[0][0] - body[1][0];
        int dy = body[0][1] - body[1][1];
        if (dx == 1 || dx == -11) { dir = 3; nextDir = 3; }
        else if (dx == -1 || dx == 11) { dir = 2; nextDir = 2; }
        else if (dy == 1 || dy == -7) { dir = 1; nextDir = 1; }
        else if (dy == -1 || dy == 7) { dir = 0; nextDir = 0; }
      }
      
      if (gameMode == 5) {
        determineAppleCount();
        spawnAllApples();
      } else {
        spawnSingleApple(eatenApple);
      }
    }

    drawGame();
    lastMoveTime = millis(); 
  }
}