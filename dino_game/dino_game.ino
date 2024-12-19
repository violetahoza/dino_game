#include <Wire.h>  // I2C communication
#include <Adafruit_GFX.h> // provides graphics functions for display
#include <Adafruit_SSD1306.h> // manages the OLED display

#define SCREEN_WIDTH 128 // width of the OLED screen in pixels
#define SCREEN_HEIGHT 64 // height of the OLED screen in pixels
#define OLED_RESET -1 // OLED reset pin 
#define BUTTON_PIN 2 // pin for the button to control the dinosaur
#define BUZZER_PIN 3 // pin for the buzzer (used for sound effects)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // create the display object

// Game states
enum GameState {
  START_SCREEN, // game is on the start screen
  PLAYING, // game is ongoing (playing)
  GAME_OVER // Game has ended
};

// Variable that holds the current state of the game
GameState gameState = START_SCREEN; // initialize the game state to the start screen

// Game constants (in pixels)
const int DINO_WIDTH = 25;
const int DINO_HEIGHT = 26;
const int GROUND_HEIGHT = 54; // height at which the ground is drawn on the screen
const int CACTUS_WIDTH = 12;
const int CACTUS_HEIGHT = 24;
const int SMALL_CACTUS_WIDTH = 10;  
const int SMALL_CACTUS_HEIGHT = 20; 

// Game variables
int dinoY; // stores the vertical position of the dinosaur
float dinoVelocity; // the dinosaur's speed while jumping
bool isJumping = false; // checks if the dinosaur is in mid-air.
int score = 0; // holds the player's current score
int highestScore = 0; // stores the best score

// Background elements
struct Cloud {
  int x; // x-coordinate of the cloud
  int y; // y-coordinate of the cloud
  bool active; // flag to check if the cloud is active or not
};

struct Star {
  int x; // x-coordinate of the star
  int y; // y-coordinate of the star
  bool active; // flag to check if the star is active or not
  unsigned long lastBlink; // last time the star blinked
  bool visible; // flag to track if the star is visible or not
};

const int MAX_CLOUDS = 3; // maximum number of clouds
const int MAX_STARS = 8; // maximum number of stars
Cloud clouds[MAX_CLOUDS]; // array to hold clouds
Star stars[MAX_STARS]; // array to hold stars

// Obstacle management
const int MAX_OBSTACLES = 3; // maximum number of obstacles
int obstacleX[MAX_OBSTACLES]; // x-coordinates of the obstacles
bool obstacleActive[MAX_OBSTACLES];  // flags to check if obstacles are active
bool obstacleIsSmall[MAX_OBSTACLES]; // keep track of which obstacles are small

// Bitmap definitions for game elements (dino, cactus, clouds)
const unsigned char PROGMEM dino[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x06, 0xff, 0x00, 0x00, 0x0e, 0xff, 0x00, 
  0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
  0x00, 0x0f, 0xfc, 0x00, 0x40, 0x0f, 0xc0, 0x00, 0x40, 0x1f, 0x80, 0x00, 0x40, 0x7f, 0x80, 0x00, 
  0x60, 0xff, 0xe0, 0x00, 0x71, 0xff, 0xa0, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0x00, 
  0x7f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 
  0x03, 0xfc, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 
  0x01, 0x0c, 0x00, 0x00, 0x01, 0x8e, 0x00, 0x00
};

// 12x24 cactus
const unsigned char PROGMEM cactus[] = {
  0x1e, 0x00, 0x1f, 0x00, 0x1f, 0x40, 0x1f, 0xe0, 0x1f, 0xe0, 0xdf, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 
  0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xc0, 0xff, 0x00, 0xff, 0x00, 0x7f, 0x00, 
  0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00
};

// 10x20 cactus
const unsigned char PROGMEM smallCactus[] = {
  0x30, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x01, 0xFB, 0x03,
  0xFF, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0xFF, 0x01,
  0xFE, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00,
  0x78, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM cloud[] = {
    0x0F, 0xC0, 0x1F, 0xE0, 0x3F, 0xF0, 0x3F, 0xF0, 0x1F, 0xE0, 0x0F, 0xC0
};

void setup() {
  Serial.begin(9600); // start serial communication for debugging
  pinMode(BUTTON_PIN, INPUT_PULLUP); // configure the button pin as input with internal pull-up
  pinMode(BUZZER_PIN, OUTPUT); // configure the buzzer pin as output
  
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // stop the program if the display initialization fails
  }
  display.clearDisplay(); // clear the display buffer
  display.display(); // update the display
  
  randomSeed(analogRead(0)); // initialize random seed using analog noise
  initializeGame(); // call function to initialize game variables
}

// Initializes the game variables to their starting values
void initializeGame() {
  dinoY = GROUND_HEIGHT - DINO_HEIGHT; // set dino's Y-position to ground level
  dinoVelocity = 0; // set the initial vertical velocity to 0
  isJumping = false; // the dino is not jumping
  score = 0; // reset the score
  
  // Initialize clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    clouds[i].active = false; // set cloud as inactive initially
    clouds[i].x = SCREEN_WIDTH + (i * SCREEN_WIDTH/2); // position the cloud off-screen on the right 
    clouds[i].y = random(0, GROUND_HEIGHT - 30); // randomize the cloud's vertical position
  }
  
  // Initialize stars
  for(int i = 0; i < MAX_STARS; i++) {
    stars[i].active = true;
    stars[i].x = random(0, SCREEN_WIDTH); // randomize the horizontal position of the stars
    stars[i].y = random(0, GROUND_HEIGHT - 40); // randomize the vertical position of  the stars
    stars[i].lastBlink = millis(); // set the initial blink time
    stars[i].visible = true; // all stars are visible initially
  }
  
  // Initialize obstacles
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    obstacleActive[i] = false; // set all obstacles to inactive
    obstacleX[i] = SCREEN_WIDTH + (i * SCREEN_WIDTH/2); // position obstacles off-screen on the right 
    obstacleIsSmall[i] = random(2) == 0; // 50% chance to create a small cactus
  }
}

// Draws the start screen
void drawStartScreen() {
  display.clearDisplay(); // clear the display
  
  // Draw title
  display.setTextSize(2); // set text size
  display.setTextColor(WHITE); // set text color to white
  display.setCursor(15, 10); // set cursor position
  display.print("DINO RUN");
  
  // Draw instructions
  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print("Push button to play");
  
  // Draw highest score
  display.setCursor(10, 50);
  display.print("Highest Score: ");
  display.print(highestScore);
  
  display.display(); // update the display with the drawn elements
}

// Updates the background (clouds and stars)
void updateBackground() {
  // Update clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    if (clouds[i].active) {
      clouds[i].x -= 1; // move the cloud to the left
      if (clouds[i].x < -20) { // if cloud is off the screen
        clouds[i].active = false; // deactivate the cloud
      }
    } else if (random(100) < 1) { // random chance to activate a cloud
      clouds[i].active = true;
      clouds[i].x = SCREEN_WIDTH; // reposition the cloud to the right side
      clouds[i].y = random(0, GROUND_HEIGHT - 30); // random vertical position
    }
  }
  
  // Update stars (blinking effect)
  unsigned long currentTime = millis();
  for(int i = 0; i < MAX_STARS; i++) {
    if (currentTime - stars[i].lastBlink > 1000 + random(2000)) {
      stars[i].visible = !stars[i].visible; // toggle star visibility
      stars[i].lastBlink = currentTime; // update the last blink time
    }
  }
}

// Draws the background (clouds and stars)
void drawBackground() {
  // Draw stars
  for(int i = 0; i < MAX_STARS; i++) {
    if (stars[i].active && stars[i].visible) { // only draw active and visible stars
      display.drawPixel(stars[i].x, stars[i].y, WHITE);
    }
  }
  
  // Draw clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    if (clouds[i].active) {
      display.drawBitmap(clouds[i].x, clouds[i].y, cloud, 16, 6, WHITE); // draw the cloud bitmap
    }
  }
}

// Draws the obstacles 
void drawObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      if (obstacleIsSmall[i]) {
        display.drawBitmap(obstacleX[i], GROUND_HEIGHT - SMALL_CACTUS_HEIGHT, smallCactus, SMALL_CACTUS_WIDTH, SMALL_CACTUS_HEIGHT, WHITE); // draw small cactus
      } else {
        display.drawBitmap(obstacleX[i], GROUND_HEIGHT - CACTUS_HEIGHT, cactus, CACTUS_WIDTH, CACTUS_HEIGHT, WHITE); // draw large cactus
      }
    }
  }
}


// Updates the obstacles
void updateObstacles() {
  // Move obstacles leftward, and reset them if they move off-screen
  // Randomly generate new obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      if (obstacleIsSmall[i]) {
        obstacleX[i] -= 3;  
      } else {
        obstacleX[i] -= 4;  
      }

      if (obstacleX[i] < -CACTUS_WIDTH) {
        obstacleActive[i] = false;
        score++;
      }
    } else if (random(100) < 2) {  // adjust the spawn rate
      obstacleActive[i] = true;
      obstacleX[i] = SCREEN_WIDTH;
      obstacleIsSmall[i] = random(2) == 0;  // 50% chance for small cactus
    }
  }
}

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}

// Updates the game logic
void updateGame() {
  // Jump mechanics
  if (!isJumping && digitalRead(BUTTON_PIN) == LOW) {
    isJumping = true;
    dinoVelocity = -8.0;
    playTone(800, 50); // play jump sound
  }
  
  if (isJumping) {
    dinoY += dinoVelocity; // update the dinosaur's position based on jump velocity
    dinoVelocity += 0.6; // apply gravity (increasing downward velocity)
    
    // Check if the dinosaur touches the ground
    if (dinoY >= GROUND_HEIGHT - DINO_HEIGHT) {
      dinoY = GROUND_HEIGHT - DINO_HEIGHT;
      dinoVelocity = 0;
      isJumping = false;
    }
  }
  
  // Update obstacles
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      obstacleX[i] -= 4;
      if (obstacleX[i] < -CACTUS_WIDTH) {
        obstacleActive[i] = false;
        score++;
      }
    } else if (random(100) < 2) {
      obstacleActive[i] = true;
      obstacleX[i] = SCREEN_WIDTH;
    }
  }
  
  // Update background elements
  updateBackground();
  
  // Check collisions
  if (checkCollision()) {
    gameState = GAME_OVER;
    playTone(200, 500);
    if (score > highestScore) {
      highestScore = score;
    }
  }
}

// Checks if the dinosaur collides with any obstacle
bool checkCollision() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      int obstacleWidth = obstacleIsSmall[i] ? SMALL_CACTUS_WIDTH : CACTUS_WIDTH;
      int obstacleHeight = obstacleIsSmall[i] ? SMALL_CACTUS_HEIGHT : CACTUS_HEIGHT;

      if (obstacleX[i] < (DINO_WIDTH) &&
          (obstacleX[i] + obstacleWidth) > 0 &&
          (dinoY + DINO_HEIGHT) > (GROUND_HEIGHT - obstacleHeight)) {
        return true;
      }
    }
  }
  return false;
}

// Shows game over screen with final score and highest score
void drawGameOver() {
  display.clearDisplay();
  
  if (gameState == GAME_OVER) {
    // Draw only game over screen
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(8, 15);
    display.print("Game Over!");
    
    // Display final score
    display.setTextSize(1);
    display.setCursor(10, 35);
    display.print("Score: ");
    display.print(score);
    
    // Display high score
    display.setCursor(10, 45);
    display.print("Highest Score: ");
    display.print(highestScore);
    
    display.setCursor(10, 55);
    display.print("Push button to play");
  } else {
    // Draw background
    drawBackground();
    
    // Draw dino
    display.drawBitmap(0, dinoY, dino, DINO_WIDTH, DINO_HEIGHT, WHITE);
    
    // Draw ground
    display.drawLine(0, GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, WHITE);
    
    // Draw obstacles
    drawObstacles();
    
    // Draw score during gameplay
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(SCREEN_WIDTH - 40, 0);
    display.print(score);
  }
  
  display.display();
}

// Main game loop
void loop() {
  switch (gameState) {
    case START_SCREEN:
      drawStartScreen(); // draw the start screen
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(200);  // button debounce
        gameState = PLAYING; // start the game
        initializeGame(); // reset game variables
      }
      break;
      
    case PLAYING:
      updateGame(); // update game logic (obstacles, background, score, etc.)
      drawGameOver(); // draw the game elements
      break;
      
    case GAME_OVER:
      drawGameOver(); // draw the game over screen
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(200);  // button debounce
        gameState = START_SCREEN; // go back to start screen
      }
      break;
  }
  
  delay(16);  // Approximately 60 FPS
}