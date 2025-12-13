#ifndef PADS_MODE_H
#define PADS_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Grid Piano mode variables (Linnstrument-style all 4ths layout)
#define GRID_COLS 8
#define GRID_ROWS 5
int gridOctave = 3;
int gridPressedNote = -1;
int gridLayout[GRID_ROWS][GRID_COLS];

// Function declarations
void initializeGridPianoMode();
void drawGridPianoMode();
void handleGridPianoMode();
void drawGridCell(int row, int col, bool pressed = false);
void calculateGridLayout();
int getGridNote(int row, int col);

// Implementations
void initializeGridPianoMode() {
  gridOctave = 3;
  gridPressedNote = -1;
  calculateGridLayout();
  
  drawGridPianoMode();
}

void calculateGridLayout() {
  // Linnstrument-style all 4ths layout
  // Each row is 5 semitones (perfect 4th) higher than the row below
  // Each column is 1 semitone higher than the column to the left
  // Row 0 = bottom (lowest notes), Row 4 = top (highest notes)
  int baseNote = 36 + (gridOctave - 3) * 12; // C3 base
  
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      // Reverse row order: (GRID_ROWS - 1 - row) makes row 0 the bottom
      gridLayout[row][col] = baseNote + col + ((GRID_ROWS - 1 - row) * 5);
    }
  }
}

void drawGridPianoMode() {
  tft.fillScreen(THEME_BG);
  drawModuleHeader("PADS");
  
  // Calculate grid area from screen dimensions
  int startX = 10;
  int startY = CONTENT_TOP + 5;
  int spacing = 2;
  int availableWidth = SCREEN_WIDTH - (2 * startX);
  int availableHeight = SCREEN_HEIGHT - startY - 60; // Leave space for controls
  int cellW = (availableWidth - (GRID_COLS - 1) * spacing) / GRID_COLS;
  int cellH = (availableHeight - (GRID_ROWS - 1) * spacing) / GRID_ROWS;
  
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      drawGridCell(row, col);
    }
  }
  
  // Octave controls - positioned at bottom with spacing
  int ctrlY = SCREEN_HEIGHT - 50;
  int btnSpacing = 10;
  drawRoundButton(btnSpacing, ctrlY, 60, 35, "OCT-", THEME_SECONDARY);
  drawRoundButton(btnSpacing * 2 + 60, ctrlY, 60, 35, "OCT+", THEME_SECONDARY);
  
  // Octave display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  tft.drawString("Oct " + String(gridOctave), btnSpacing * 3 + 120, ctrlY + 10, 2);
  
  // Current note display
  if (gridPressedNote != -1) {
    tft.setTextColor(THEME_PRIMARY, THEME_BG);
    tft.drawString("Playing: " + getNoteNameFromMIDI(gridPressedNote), SCREEN_WIDTH / 2, ctrlY + 10, 1);
  }
}

void drawGridCell(int row, int col, bool pressed) {
  // Calculate dimensions dynamically
  int startX = 10;
  int startY = CONTENT_TOP + 5;
  int spacing = 2;
  int availableWidth = SCREEN_WIDTH - (2 * startX);
  int availableHeight = SCREEN_HEIGHT - startY - 60;
  int cellW = (availableWidth - (GRID_COLS - 1) * spacing) / GRID_COLS;
  int cellH = (availableHeight - (GRID_ROWS - 1) * spacing) / GRID_ROWS;
  
  int x = startX + col * (cellW + spacing);
  int y = startY + row * (cellH + spacing);
  
  int note = gridLayout[row][col];
  int noteInOctave = note % 12;
  bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);
  
  uint16_t bgColor, textColor;
  
  if (pressed) {
    bgColor = THEME_PRIMARY;
    textColor = THEME_BG;
  } else if (isBlackKey) {
    bgColor = THEME_SURFACE;
    textColor = THEME_TEXT_DIM;  // Lighter text for accidentals
  } else {
    bgColor = THEME_BG;
    textColor = THEME_TEXT;
  }
  
  tft.fillRect(x, y, cellW, cellH, bgColor);
  tft.drawRect(x, y, cellW, cellH, THEME_PRIMARY);
  
  // Note name
  String noteName = getNoteNameFromMIDI(note);
  tft.setTextColor(textColor, bgColor);
  tft.drawCentreString(noteName, x + cellW/2, y + cellH/2 - 6, 1);
}

void handleGridPianoMode() {
  // Back button - larger touch area
  if (touch.justPressed && isButtonPressed(BACK_BTN_X, BACK_BTN_Y, BTN_BACK_W, BTN_BACK_H)) {
    exitToMenu();
    return;
  }
  
  // Calculate button positions
  int ctrlY = SCREEN_HEIGHT - 50;
  int btnSpacing = 10;
  
  if (touch.justPressed) {
    // Octave controls
    if (isButtonPressed(btnSpacing, ctrlY, 60, 35)) {
      gridOctave = max(1, gridOctave - 1);
      calculateGridLayout();
      drawGridPianoMode();
      return;
    }
    if (isButtonPressed(btnSpacing * 2 + 60, ctrlY, 60, 35)) {
      gridOctave = min(6, gridOctave + 1);
      calculateGridLayout();
      drawGridPianoMode();
      return;
    }
  }
  
  // Grid interaction - recalculate dimensions
  int startX = 10;
  int startY = CONTENT_TOP + 5;
  int spacing = 2;
  int availableWidth = SCREEN_WIDTH - (2 * startX);
  int availableHeight = SCREEN_HEIGHT - startY - 60;
  int cellW = (availableWidth - (GRID_COLS - 1) * spacing) / GRID_COLS;
  int cellH = (availableHeight - (GRID_ROWS - 1) * spacing) / GRID_ROWS;
  
  int pressedNote = -1;
  
  if (touch.isPressed) {
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        int x = startX + col * (cellW + spacing);
        int y = startY + row * (cellH + spacing);
        
        if (isButtonPressed(x, y, cellW, cellH)) {
          pressedNote = gridLayout[row][col];
          drawGridCell(row, col, true);
          break;
        }
      }
      if (pressedNote != -1) break;
    }
  }
  
  // Handle note changes
  if (pressedNote != gridPressedNote) {
    // Turn off old note
    if (gridPressedNote != -1) {
      sendNoteOff(gridPressedNote);
      // Redraw old cell
      for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
          if (gridLayout[row][col] == gridPressedNote) {
            drawGridCell(row, col, false);
            break;
          }
        }
      }
    }
    
    // Turn on new note
    if (pressedNote != -1) {
      sendNoteOn(pressedNote, 100);
    }
    
    gridPressedNote = pressedNote;
    
    // Update display
    if (gridPressedNote != -1) {
      tft.fillRect(180, 200, 140, 16, THEME_BG);
      tft.setTextColor(THEME_PRIMARY, THEME_BG);
      tft.drawString("Playing: " + getNoteNameFromMIDI(gridPressedNote), 180, 207, 1);
    } else {
      tft.fillRect(180, 200, 140, 16, THEME_BG);
    }
  }
}

#endif