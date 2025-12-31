#ifndef BOUNCING_BALL_MODE_H
#define BOUNCING_BALL_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Pong-style Ambient MIDI mode variables
struct Ball {
  float x, y;
  float vx, vy;
  uint16_t color;
  int size;
  bool active;
};

// Play area constants - calculated based on screen size
#define PLAY_AREA_MARGIN_X (SCREEN_WIDTH / 6)
#define PLAY_AREA_MARGIN_Y_TOP (CONTENT_TOP + 20)
#define PLAY_AREA_MARGIN_Y_BOTTOM (SCREEN_HEIGHT > 250 ? 100 : 80)
#define WALL_THICKNESS 4

#define MAX_BALLS 4
Ball balls[MAX_BALLS];
int numActiveBalls = 1;

// Simple wall system - notes triggered by wall hits
struct Wall {
  int x, y, w, h;
  int note;
  String noteName;
  uint16_t color;
  bool active;
  unsigned long activeTime;
  int side; // 0=top, 1=right, 2=bottom, 3=left
};

#define NUM_WALLS 24  // 8 top + 8 bottom + 4 left + 4 right
Wall walls[NUM_WALLS];
int ballScale = 0;  // Scale selection
int ballKey = 0;    // Key selection
int ballOctave = 4;

// Function declarations
void initializeBouncingBallMode();
void drawBouncingBallMode();
void handleBouncingBallMode();
void initializeBalls();
void initializeWalls();
void updateBouncingBall();
void updateBalls();
void drawBalls();
void drawWalls();
void checkWallCollisions();

// Implementations
void initializeBouncingBallMode() {
  ballScale = 0;
  ballKey = 0;
  ballOctave = 4;
  numActiveBalls = 1;
  initializeBalls();
  initializeWalls();
  
  drawBouncingBallMode();
}

void drawBouncingBallMode() {
  tft.fillScreen(THEME_BG);
  drawModuleHeader("ZEN");
  
  // Calculate control button layout from screen dimensions
  int btnSpacing = 10;
  int btnY = SCREEN_HEIGHT - 50;
  int btnH = 30;
  int statusY = SCREEN_HEIGHT - 75;
  drawRoundButton(10, btnY, 50, 30, "ADD", THEME_SUCCESS, false);
  drawRoundButton(70, btnY, 60, 30, "RESET", THEME_WARNING, false);
  drawRoundButton(140, btnY, 60, 30, "SCALE", THEME_ACCENT, false);
  drawRoundButton(210, btnY, 50, 30, "KEY-", THEME_SECONDARY, false);
  drawRoundButton(270, btnY, 50, 30, "KEY+", THEME_SECONDARY, false);
  drawRoundButton(330, btnY, 50, 30, "OCT", THEME_PRIMARY, false);
  
  // Status display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String keyName = getNoteNameFromMIDI(ballKey);
  tft.drawString(keyName + " " + scales[ballScale].name, btnSpacing, statusY, 2);
  tft.drawString("Oct:" + String(ballOctave), SCREEN_WIDTH / 2, statusY, 2);
  tft.drawString("Balls:" + String(numActiveBalls), SCREEN_WIDTH - 80, statusY, 2);
  
  drawWalls();
  drawBalls();
}

void initializeBalls() {
  const int playLeft = PLAY_AREA_MARGIN_X;
  const int playRight = SCREEN_WIDTH - PLAY_AREA_MARGIN_X;
  const int playTop = PLAY_AREA_MARGIN_Y_TOP;
  const int playBottom = SCREEN_HEIGHT - PLAY_AREA_MARGIN_Y_BOTTOM;
  
  for (int i = 0; i < MAX_BALLS; i++) {
    balls[i].x = random(playLeft + 20, playRight - 20);
    balls[i].y = random(playTop + 20, playBottom - 20);
    // Slower, more zen-like movement
    balls[i].vx = random(-15, 15) / 10.0; // -1.5 to 1.5
    balls[i].vy = random(-15, 15) / 10.0;
    if (abs(balls[i].vx) < 0.5) balls[i].vx = (balls[i].vx >= 0) ? 0.8 : -0.8;
    if (abs(balls[i].vy) < 0.5) balls[i].vy = (balls[i].vy >= 0) ? 0.8 : -0.8;
    // Softer, more zen colors
    balls[i].color = random(0x2000, 0x8FFF);
    balls[i].size = random(4, 7);
    balls[i].active = (i < numActiveBalls);
  }
}

void initializeWalls() {
  int wallIndex = 0;
  
  const int playLeft = PLAY_AREA_MARGIN_X;
  const int playRight = SCREEN_WIDTH - PLAY_AREA_MARGIN_X;
  const int playTop = PLAY_AREA_MARGIN_Y_TOP;
  const int playBottom = SCREEN_HEIGHT - PLAY_AREA_MARGIN_Y_BOTTOM;
  const int playWidth = playRight - playLeft;
  const int playHeight = playBottom - playTop;
  
  // Top wall - 8 segments
  const int segmentWidth = playWidth / 8;
  for (int i = 0; i < 8; i++) {
    walls[wallIndex].x = playLeft + i * segmentWidth;
    walls[wallIndex].y = playTop;
    walls[wallIndex].w = segmentWidth;
    walls[wallIndex].h = WALL_THICKNESS;
    walls[wallIndex].note = getNoteInScale(ballScale, i, ballOctave) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_PRIMARY;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 0;
    wallIndex++;
  }
  
  // Right wall - 4 segments
  const int segmentHeight = playHeight / 4;
  for (int i = 0; i < 4; i++) {
    walls[wallIndex].x = playRight - WALL_THICKNESS;
    walls[wallIndex].y = playTop + i * segmentHeight;
    walls[wallIndex].w = WALL_THICKNESS;
    walls[wallIndex].h = segmentHeight;
    walls[wallIndex].note = getNoteInScale(ballScale, i, ballOctave + 1) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_SECONDARY;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 1;
    wallIndex++;
  }
  
  // Bottom wall - 8 segments
  for (int i = 0; i < 8; i++) {
    walls[wallIndex].x = playLeft + i * segmentWidth;
    walls[wallIndex].y = playBottom - WALL_THICKNESS;
    walls[wallIndex].w = segmentWidth;
    walls[wallIndex].h = WALL_THICKNESS;
    walls[wallIndex].note = getNoteInScale(ballScale, 7 - i, ballOctave) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_ACCENT;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 2;
    wallIndex++;
  }
  
  // Left wall - 4 segments
  for (int i = 0; i < 4; i++) {
    walls[wallIndex].x = playLeft;
    walls[wallIndex].y = playTop + i * segmentHeight;
    walls[wallIndex].w = WALL_THICKNESS;
    walls[wallIndex].h = segmentHeight;
    walls[wallIndex].note = getNoteInScale(ballScale, 3 - i, ballOctave + 1) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_WARNING;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 3;
    wallIndex++;
  }
}

void handleBouncingBallMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button layout from screen dimensions
  int btnSpacing = 10;
  int btnY = SCREEN_HEIGHT - 50;
  int btnH = 30;
  int availableWidth = SCREEN_WIDTH - (2 * btnSpacing);
  int btn1W = (availableWidth - (5 * btnSpacing)) / 6;
  
  // Draw control buttons with press feedback
  bool addPressed = touch.isPressed && isButtonPressed(10, btnY, 50, 30);
  bool resetPressed = touch.isPressed && isButtonPressed(70, btnY, 60, 30);
  bool scalePressed = touch.isPressed && isButtonPressed(140, btnY, 60, 30);
  bool keyDownPressed = touch.isPressed && isButtonPressed(210, btnY, 50, 30);
  bool keyUpPressed = touch.isPressed && isButtonPressed(270, btnY, 50, 30);
  bool octPressed = touch.isPressed && isButtonPressed(330, btnY, 50, 30);
  
  // Only redraw buttons when pressed
  if (addPressed || resetPressed || scalePressed || keyDownPressed || keyUpPressed || octPressed) {
    drawRoundButton(10, btnY, 50, 30, "ADD", THEME_SUCCESS, addPressed);
    drawRoundButton(70, btnY, 60, 30, "RESET", THEME_WARNING, resetPressed);
    drawRoundButton(140, btnY, 60, 30, "SCALE", THEME_ACCENT, scalePressed);
    drawRoundButton(210, btnY, 50, 30, "KEY-", THEME_SECONDARY, keyDownPressed);
    drawRoundButton(270, btnY, 50, 30, "KEY+", THEME_SECONDARY, keyUpPressed);
    drawRoundButton(330, btnY, 50, 30, "OCT", THEME_PRIMARY, octPressed);
  }
  
  if (touch.justPressed) {
    // Add ball button
    if (addPressed) {
      if (numActiveBalls < MAX_BALLS) {
        numActiveBalls++;
        initializeBalls();
        drawBouncingBallMode();
      }
      return;
    }
    
    // Reset button
    if (resetPressed) {
      numActiveBalls = 1;
      initializeBalls();
      drawBouncingBallMode();
      return;
    }
    
    // Scale button
    if (scalePressed) {
      ballScale = (ballScale + 1) % NUM_SCALES;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    // Key controls
    if (keyDownPressed) {
      ballKey = (ballKey - 1 + 12) % 12;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    if (keyUpPressed) {
      ballKey = (ballKey + 1) % 12;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    // Octave button
    if (octPressed) {
      ballOctave = (ballOctave == 7) ? 2 : ballOctave + 1;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
  }
  
  // Update physics and display
  updateBouncingBall();
}

void updateBouncingBall() {
  // Smooth 60 FPS animation
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 16) {
    const int playLeft = PLAY_AREA_MARGIN_X;
    const int playRight = SCREEN_WIDTH - PLAY_AREA_MARGIN_X;
    const int playTop = PLAY_AREA_MARGIN_Y_TOP;
    const int playBottom = SCREEN_HEIGHT - PLAY_AREA_MARGIN_Y_BOTTOM;
    
    // Clear entire play area to prevent flickering
    tft.fillRect(playLeft + WALL_THICKNESS, playTop + WALL_THICKNESS, 
                 playRight - playLeft - 2 * WALL_THICKNESS, 
                 playBottom - playTop - 2 * WALL_THICKNESS, THEME_BG);
    
    updateBalls();
    checkWallCollisions();
    
    // Draw walls
    drawWalls();
    
    // Draw balls
    drawBalls();
    
    lastUpdate = millis();
  }
}

void updateBalls() {
  const int playLeft = PLAY_AREA_MARGIN_X;
  const int playRight = SCREEN_WIDTH - PLAY_AREA_MARGIN_X;
  const int playTop = PLAY_AREA_MARGIN_Y_TOP;
  const int playBottom = SCREEN_HEIGHT - PLAY_AREA_MARGIN_Y_BOTTOM;
  
  for (int i = 0; i < numActiveBalls; i++) {
    if (!balls[i].active) continue;
    
    // Update position
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;
    
    // Bounce off walls with proper collision detection
    if (balls[i].x - balls[i].size <= playLeft + WALL_THICKNESS) {
      balls[i].vx = abs(balls[i].vx);
      balls[i].x = playLeft + WALL_THICKNESS + balls[i].size;
    }
    if (balls[i].x + balls[i].size >= playRight - WALL_THICKNESS) {
      balls[i].vx = -abs(balls[i].vx);
      balls[i].x = playRight - WALL_THICKNESS - balls[i].size;
    }
    if (balls[i].y - balls[i].size <= playTop + WALL_THICKNESS) {
      balls[i].vy = abs(balls[i].vy);
      balls[i].y = playTop + WALL_THICKNESS + balls[i].size;
    }
    if (balls[i].y + balls[i].size >= playBottom - WALL_THICKNESS) {
      balls[i].vy = -abs(balls[i].vy);
      balls[i].y = playBottom - WALL_THICKNESS - balls[i].size;
    }
  }
}

void drawBalls() {
  for (int i = 0; i < numActiveBalls; i++) {
    if (!balls[i].active) continue;
    tft.fillCircle(balls[i].x, balls[i].y, balls[i].size, balls[i].color);
    tft.drawCircle(balls[i].x, balls[i].y, balls[i].size, THEME_TEXT);
  }
}

void drawWalls() {
  for (int i = 0; i < NUM_WALLS; i++) {
    uint16_t color = walls[i].color;
    
    // Bright flash when active
    if (walls[i].active) {
      unsigned long elapsed = millis() - walls[i].activeTime;
      if (elapsed < 200) {
        color = THEME_TEXT; // Bright white flash
      } else {
        walls[i].active = false;
      }
    }
    
    // Draw wall
    tft.fillRect(walls[i].x, walls[i].y, walls[i].w, walls[i].h, color);
    
    // Add note name for horizontal walls (top and bottom)
    if (walls[i].w > walls[i].h && walls[i].w > 30) {
      tft.setTextColor(THEME_BG, color);
      tft.drawCentreString(walls[i].noteName, 
                          walls[i].x + walls[i].w/2, 
                          walls[i].y - 2, 1);
    }
  }
}


void checkWallCollisions() {
  for (int b = 0; b < numActiveBalls; b++) {
    if (!balls[b].active) continue;
    
    static float lastX[MAX_BALLS], lastY[MAX_BALLS];
    static bool initialized = false;
    
    if (!initialized) {
      for (int i = 0; i < MAX_BALLS; i++) {
        lastX[i] = balls[i].x;
        lastY[i] = balls[i].y;
      }
      initialized = true;
    }
    
    // Check collision with each wall segment
    for (int w = 0; w < NUM_WALLS; w++) {
      if (walls[w].active) continue; // Skip if wall is already active
      
      bool collision = false;
      
      // Check collision based on ball position and wall bounds
      if (walls[w].side == 0) { // Top walls
        if (balls[b].y - balls[b].size <= walls[w].y + walls[w].h &&
            balls[b].x >= walls[w].x && balls[b].x <= walls[w].x + walls[w].w &&
            lastY[b] > balls[b].y) {
          collision = true;
        }
      }
      else if (walls[w].side == 1) { // Right walls
        if (balls[b].x + balls[b].size >= walls[w].x &&
            balls[b].y >= walls[w].y && balls[b].y <= walls[w].y + walls[w].h &&
            lastX[b] < balls[b].x) {
          collision = true;
        }
      }
      else if (walls[w].side == 2) { // Bottom walls
        if (balls[b].y + balls[b].size >= walls[w].y &&
            balls[b].x >= walls[w].x && balls[b].x <= walls[w].x + walls[w].w &&
            lastY[b] < balls[b].y) {
          collision = true;
        }
      }
      else if (walls[w].side == 3) { // Left walls
        if (balls[b].x - balls[b].size <= walls[w].x + walls[w].w &&
            balls[b].y >= walls[w].y && balls[b].y <= walls[w].y + walls[w].h &&
            lastX[b] > balls[b].x) {
          collision = true;
        }
      }
      
      if (collision) {
        int velocity = random(70, 110);
        sendNoteOn(walls[w].note, velocity);
        sendNoteOff(walls[w].note);
        
        walls[w].active = true;
        walls[w].activeTime = millis();
        
        Serial.printf("Wall segment hit: %s\n", walls[w].noteName.c_str());
        break; // Only trigger one wall per ball per frame
      }
    }
    
    // Update last positions
    lastX[b] = balls[b].x;
    lastY[b] = balls[b].y;
  }
}

#endif