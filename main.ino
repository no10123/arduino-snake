#include "Arduino_LED_Matrix.h"
#define MAX_LENGTH 96

ArduinoLEDMatrix matrix;

byte frame[8][12] = {0};

unsigned long lastMoveTime = 0;
const int gameSpeed = 250; 

int body[MAX_LENGTH][2]; 
int apple[2] = {5, 4};
const int btnUp = 3;
const int btnDown = 2;
const int btnLeft = 5;
const int btnRight = 4;

int dir = 3;     
int nextDir = 3; 
int length = 3;

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

void spawnApple() {
  bool onBody;
  do {
    onBody = false;
    apple[0] = random(0, 12); 
    apple[1] = random(0, 8);
    
    for (int i = 0; i < length; i++) {
      if (body[i][0] == apple[0] && body[i][1] == apple[1]) {
        onBody = true;
        break;
      }
    }
  } while (onBody == true);
  
  writePixel(apple[0], apple[1], 1); 
}

void resetGame() {
  clear();
  length = 3;
  dir = 3;
  nextDir = 3;
  
  body[0][0] = 3; body[0][1] = 4; 
  body[1][0] = 2; body[1][1] = 4; 
  body[2][0] = 1; body[2][1] = 4; 
  
  for (int i = 0; i < length; i++) {
    writePixel(body[i][0], body[i][1], 1);
  }
  
  spawnApple();
  refreshMatrix(); 
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

    if (dir == 0) body[0][1]--; 
    if (dir == 1) body[0][1]++; 
    if (dir == 2) body[0][0]--; 
    if (dir == 3) body[0][0]++; 

    body[0][0] = (body[0][0] + 12) % 12;
    body[0][1] = (body[0][1] + 8) % 8;

    for (int i = 1; i < length; i++) {
      if (body[0][0] == body[i][0] && body[0][1] == body[i][1]) {
        delay(500);
        resetGame();
        return; 
      }
    }

    writePixel(body[0][0], body[0][1], 1);

    if (body[0][0] == apple[0] && body[0][1] == apple[1]) {
      if (length < MAX_LENGTH) {
        body[length][0] = oldTailX;
        body[length][1] = oldTailY;
        length++; 
      }
      spawnApple(); 
    } else {
      writePixel(oldTailX, oldTailY, 0);
    }

    refreshMatrix();

    lastMoveTime = millis(); 
  }
}