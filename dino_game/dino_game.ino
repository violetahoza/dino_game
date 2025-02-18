#include <Wire.h>  // I2C communication
#include <Adafruit_GFX.h> // provides graphics functions for display
#include <Adafruit_SSD1306.h> // manages the OLED display
#include <TM1637Display.h> 

#define SCREEN_WIDTH 128 // width of the OLED screen in pixels
#define SCREEN_HEIGHT 64 // height of the OLED screen in pixels
#define OLED_RESET -1 // OLED reset pin 
#define JUMP_BUTTON_PIN 2 // pin for the jump button
#define DUCK_BUTTON_PIN 6 // pin for the duck button
#define BUZZER_PIN 3 // pin for the buzzer (used for sound effects)
#define CLK_PIN 4  // clock pin for TM1637
#define DIO_PIN 5  // data pin for TM1637

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // create the display object
TM1637Display timeDisplay(CLK_PIN, DIO_PIN); // create TM1637 display object

// Game states
enum GameState {
  START_SCREEN, // game is on the start screen
  PLAYING, // game is ongoing (playing)
  GAME_OVER // game has ended
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
const int PTERO_WIDTH = 23;
const int PTERO_HEIGHT = 20;
const int DINO_DUCK_WIDTH = 29;
const int DINO_DUCK_HEIGHT = 15;

// Constants for game mechanics
const int PTERO_GROUND_HEIGHT = GROUND_HEIGHT - PTERO_HEIGHT; // ground level for pterodactyl
const int PTERO_HIGH_HEIGHT = GROUND_HEIGHT - DINO_DUCK_HEIGHT - PTERO_HEIGHT - 5; // height above ducking dino
const float BASE_OBSTACLE_SPEED = 4.0; // base speed for obstacles

// Game variables
int dinoY; // stores the vertical position of the dinosaur
float dinoVelocity; // the dinosaur's speed while jumping
bool isJumping = false; // checks if the dinosaur is in mid-air
bool isDucking = false; // checks if the dinosaur is ducking
int score = 0; // holds the player's current score
int highestScore = 0; // stores the best score
unsigned long gameStartTime = 0; // when the current game started
unsigned long currentGameTime = 0; // current time in the game
unsigned long highestGameTime = 0; // highest time achieved

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
enum ObstacleType {
  CACTUS_LARGE,
  CACTUS_SMALL,
  PTERODACTYL
};

struct Obstacle {
  int x;
  bool active;
  ObstacleType type;
  int y; // for flying obstacles
};

const int MAX_OBSTACLES = 2; // maximum number of obstacles
Obstacle obstacles[MAX_OBSTACLES];

// int obstacleX[MAX_OBSTACLES]; // x-coordinates of the obstacles
// bool obstacleActive[MAX_OBSTACLES];  // flags to check if obstacles are active
// bool obstacleIsSmall[MAX_OBSTACLES]; // keep track of which obstacles are small

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

// Add running animation frames for the dinosaur
const unsigned char PROGMEM dinoRun1[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x06, 0xff, 0x00, 0x00, 0x0e, 0xff, 0x00, 
  0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
  0x00, 0x0f, 0xfc, 0x00, 0x40, 0x0f, 0xc0, 0x00, 0x40, 0x1f, 0x80, 0x00, 0x40, 0x7f, 0x80, 0x00, 
  0x60, 0xff, 0xe0, 0x00, 0x71, 0xff, 0xa0, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0x00, 
  0x7f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 
  0x03, 0xfc, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 
  0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM dinoRun2[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x06, 0xff, 0x00, 0x00, 0x0e, 0xff, 0x00, 
  0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
  0x00, 0x0f, 0xfc, 0x00, 0x40, 0x0f, 0xc0, 0x00, 0x40, 0x1f, 0x80, 0x00, 0x40, 0x7f, 0x80, 0x00, 
  0x60, 0xff, 0xe0, 0x00, 0x71, 0xff, 0xa0, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0x00, 
  0x7f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 
  0x03, 0xfc, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 
  0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM dinoDuck1[] = {
  0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x1f, 0xe0, 0x70, 0xff, 0x3f, 0xf0, 0x7f, 0xff, 0xf7, 0xf0, 
	0x3f, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xfe, 0x00, 
	0x03, 0xff, 0x9f, 0xc0, 0x01, 0xf9, 0x00, 0x00, 0x02, 0x71, 0x80, 0x00, 0x03, 0x60, 0x00, 0x00, 
	0x00, 0x40, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM dinoDuck2[] = {
  0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x1f, 0xe0, 0x70, 0xff, 0x3f, 0xf0, 0x7f, 0xff, 0xf7, 0xf0, 
	0x3f, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xfe, 0x00, 
	0x03, 0xff, 0x9f, 0xc0, 0x01, 0xf9, 0x00, 0x00, 0x03, 0x9d, 0x80, 0x00, 0x03, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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

const unsigned char PROGMEM pterodactyl1[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x60, 0x00, 0x00, 0x70, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xFC, 0x00, 0x00, 
  0xFE, 0x7F, 0x00, 0x80, 0xFF, 0x00, 0x00, 0xFF, 0x3F, 0x00, 0xFE, 0x07, 
  0x00, 0xFE, 0x1F, 0x00, 0xFE, 0x03, 0x00, 0x1E, 0x00, 0x00, 0x0E, 0x00, 
  0x00, 0x06, 0x00, 0x00, 0x06, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00
};

const unsigned char PROGMEM pterodactyl2[] = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x07, 0x00, 
  0x60, 0x0E, 0x00, 0x70, 0x1E, 0x00, 0xF8, 0x3E, 0x00, 0xFC, 0x7E, 0x00, 
  0xFE, 0x7F, 0x00, 0x80, 0xFF, 0x00, 0x00, 0xFF, 0x3F, 0x00, 0xFE, 0x07, 
  0x00, 0xFC, 0x1F, 0x00, 0xF8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Add animation variables
unsigned long lastFrameTime = 0;
const int FRAME_DELAY = 100; // time between animation frames in milliseconds
bool isFirstFrame = true;

void setup() {
  Serial.begin(9600); // start serial communication for debugging
  pinMode(JUMP_BUTTON_PIN, INPUT_PULLUP); // configure the button pin as input with internal pull-up
  pinMode(DUCK_BUTTON_PIN, INPUT_PULLUP); // configure the button pin as input with internal pull-up
  pinMode(BUZZER_PIN, OUTPUT); // configure the buzzer pin as output
  
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // stop the program if the display initialization fails
  }

  // Initialize the TM1637 display
  timeDisplay.setBrightness(0x0f); // set maximum brightness (0x00 to 0x0f)
  timeDisplay.clear();

  Serial.println(F("Dino Game initialized successfully"));
  display.clearDisplay(); // clear the display buffer
  display.display(); // update the display
  
  randomSeed(analogRead(0)); // initialize random seed using analog noise
  initializeGame(); // call function to initialize game variables
}

// Main game loop
void loop() {
  switch (gameState) {
    case START_SCREEN:
      drawStartScreen(); // draw the start screen
      displayHighestTime(); // show highest time on TM1637
      if (digitalRead(JUMP_BUTTON_PIN) == LOW || digitalRead(DUCK_BUTTON_PIN) == LOW) {
        delay(200);  // button debounce
        gameState = PLAYING; // start the game
        gameStartTime = millis(); // record game start time
        Serial.println(F("Game started!"));
        initializeGame(); // reset game variables
      }
      break;
      
    case PLAYING:
      currentGameTime = (millis() - gameStartTime) / 1000; // calculate current time in seconds
      displayCurrentTime();
      updateGame(); // update game logic (obstacles, background, score, etc.)
      drawGameOver(); // draw the game elements
      break;
      
    case GAME_OVER:
      if (currentGameTime > highestGameTime) {
        highestGameTime = currentGameTime;
        displayHighestTime();
      }
      drawGameOver(); // draw the game over screen
      if (digitalRead(JUMP_BUTTON_PIN) == LOW || digitalRead(DUCK_BUTTON_PIN) == LOW) {
        delay(200);  // button debounce
        gameState = START_SCREEN; // go back to start screen
        Serial.println(F("Returning to start screen"));
      }
      break;
  }
  
  delay(16); // Approximately 60 FPS (frames per second)
}

// Initializes the game variables to their starting values
void initializeGame() {
  dinoY = GROUND_HEIGHT - DINO_HEIGHT; // set dino's Y-position to ground level
  dinoVelocity = 0; // set the initial vertical velocity to 0
  isJumping = false; // the dino is not jumping
  isDucking = false;
  score = 0; // reset the score
  currentGameTime = 0; // reset current game time
  gameStartTime = millis();
  
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
    obstacles[i].active = false; // set all obstacles to inactive
    obstacles[i].x = SCREEN_WIDTH + (i * SCREEN_WIDTH/2); // position obstacles off-screen on the right 
    obstacles[i].type = random(3) == 0 ? PTERODACTYL : 
                       (random(2) == 0 ? CACTUS_SMALL : CACTUS_LARGE);
    obstacles[i].y = obstacles[i].type == PTERODACTYL ? 
                    random(2) * 20 + GROUND_HEIGHT - 40 : GROUND_HEIGHT;
  }

  Serial.println(F("New game initialized"));
  Serial.println(F("Current highest score: "));
  Serial.println(highestScore);
  Serial.println(F("Current highest time (seconds): "));
  Serial.println(highestGameTime);
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

// Shows game over screen with final score and highest score
void drawGameOver() {
  display.clearDisplay();
  
  if (gameState == GAME_OVER) {
    // Draw game over screen
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
    display.print("Time: ");
    display.print(currentGameTime);
    display.print("s");
    
    display.setCursor(10, 55);
    display.print("Push button to play");

    if (score > highestScore) {
      Serial.println(F("New highest score achieved!"));
      Serial.println(score);
    }
    if (currentGameTime > highestGameTime) {
      Serial.println(F("New highest time achieved!"));
      Serial.println(currentGameTime);
    }
  } else {
    drawBackground(); // draw background
    drawDino(); // draw dino
    //display.drawBitmap(0, dinoY, dino, DINO_WIDTH, DINO_HEIGHT, WHITE);
    display.drawLine(0, GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, WHITE); // draw ground
    drawObstacles(); // draw obstacles
    // Draw score during gameplay
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(SCREEN_WIDTH - 40, 0);
    display.print(score);
  }
  
  display.display();
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

// Draws the obstacles 
void drawObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      switch (obstacles[i].type) {
        case PTERODACTYL:
          // Animate pterodactyl
          if (millis() % 200 < 100) {
            display.drawBitmap(obstacles[i].x, obstacles[i].y, pterodactyl1, 
                             PTERO_WIDTH, PTERO_HEIGHT, WHITE);
          } else {
            display.drawBitmap(obstacles[i].x, obstacles[i].y, pterodactyl2, 
                             PTERO_WIDTH, PTERO_HEIGHT, WHITE);
          }
          break;
        case CACTUS_SMALL:
          display.drawBitmap(obstacles[i].x, GROUND_HEIGHT - SMALL_CACTUS_HEIGHT,
                           smallCactus, SMALL_CACTUS_WIDTH, SMALL_CACTUS_HEIGHT, WHITE);
          break;
        case CACTUS_LARGE:
          display.drawBitmap(obstacles[i].x, GROUND_HEIGHT - CACTUS_HEIGHT,
                           cactus, CACTUS_WIDTH, CACTUS_HEIGHT, WHITE);
          break;
      }
    }
  }
}

// Updates the obstacles
void updateObstacles() {
  // Update existing obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      // Move obstacle with speed based on score and obstacle type
      float speedMultiplier = 1.0 + (score / 10.0); // speed increases with score
      float baseSpeed = obstacles[i].type == PTERODACTYL ? BASE_OBSTACLE_SPEED + 1 : BASE_OBSTACLE_SPEED;
      obstacles[i].x -= (baseSpeed * speedMultiplier);

      // Remove obstacle when it goes off screen
      if (obstacles[i].x < -PTERO_WIDTH) {
        obstacles[i].active = false;
        score++;
        if (score > 0 && score % 10 == 0) {
          playMilestoneSound();
        }
      }
    } else if (random(100) < 2) {
      // Spawn new obstacle
      obstacles[i].active = true;
      obstacles[i].x = SCREEN_WIDTH;
      obstacles[i].type = random(3) == 0 ? PTERODACTYL : 
                         (random(2) == 0 ? CACTUS_SMALL : CACTUS_LARGE);
      obstacles[i].y = obstacles[i].type == PTERODACTYL ? 
                      random(2) * 20 + GROUND_HEIGHT - 40 : GROUND_HEIGHT;
    }
  }
}

// Updates the game logic
void updateGame() {
  // Handle jumping and ducking
  bool jumpPressed = (digitalRead(JUMP_BUTTON_PIN) == LOW);
  bool duckPressed = (digitalRead(DUCK_BUTTON_PIN) == LOW);
  
  // Can't jump while ducking
  if (jumpPressed && !isJumping && !isDucking) {
    isJumping = true;
    dinoVelocity = -8.0;
    playTone(800, 50);
  } 
  // Handle ducking state
  if (duckPressed && !isJumping) {
    isDucking = true;
  } else {
    isDucking = false;
  }
  // Update jump physics
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
  
  updateObstacles();
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

// Draws the dinosaur 
void drawDino() {
  unsigned long currentTime = millis();
  
  if (isDucking && !isJumping) {
    // Animate running
    if (currentTime - lastFrameTime >= FRAME_DELAY) {
      isFirstFrame = !isFirstFrame;
      lastFrameTime = currentTime;
    }
    if (isFirstFrame) {
      display.drawBitmap(0, GROUND_HEIGHT - DINO_DUCK_HEIGHT, dinoDuck1, 
                        DINO_DUCK_WIDTH, DINO_DUCK_HEIGHT, WHITE);
    } else {
      display.drawBitmap(0, GROUND_HEIGHT - DINO_DUCK_HEIGHT, dinoDuck2, 
                        DINO_DUCK_WIDTH, DINO_DUCK_HEIGHT, WHITE);
    }
    Serial.println(F("Dino ducked!"));
  } else if (isJumping) {
    // Use standard dino for jumping
    display.drawBitmap(0, dinoY, dino, DINO_WIDTH, DINO_HEIGHT, WHITE);
    Serial.println(F("Dino jumped!"));
  } else {
    // Animate running
    if (currentTime - lastFrameTime >= FRAME_DELAY) {
      isFirstFrame = !isFirstFrame;
      lastFrameTime = currentTime;
    }
    if (isFirstFrame) {
      display.drawBitmap(0, dinoY, dinoRun1, DINO_WIDTH, DINO_HEIGHT, WHITE);
    } else {
      display.drawBitmap(0, dinoY, dinoRun2, DINO_WIDTH, DINO_HEIGHT, WHITE);
    }
  }
}

// Checks if the dinosaur collides with any obstacle
bool checkCollision() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      int obstacleWidth, obstacleHeight;
      int dinoCurrentHeight = isDucking ? DINO_DUCK_HEIGHT : DINO_HEIGHT;
      int dinoCurrentWidth = isDucking ? DINO_DUCK_WIDTH : DINO_WIDTH;

      switch (obstacles[i].type) {
        case PTERODACTYL:
          obstacleWidth = PTERO_WIDTH;
          obstacleHeight = PTERO_HEIGHT;
          break;
        case CACTUS_SMALL:
          obstacleWidth = SMALL_CACTUS_WIDTH;
          obstacleHeight = SMALL_CACTUS_HEIGHT;
          break;
        case CACTUS_LARGE:
          obstacleWidth = CACTUS_WIDTH;
          obstacleHeight = CACTUS_HEIGHT;
          break;
      }

      // Check collision based on obstacle type and dino state
      if (obstacles[i].x < dinoCurrentWidth &&
          obstacles[i].x + obstacleWidth > 0) {
        
        if (obstacles[i].type == PTERODACTYL) {
          if (obstacles[i].y == PTERO_HIGH_HEIGHT) {
              // For high pterodactyl, only collide if not ducking
              if (!isDucking && dinoY + dinoCurrentHeight > obstacles[i].y) {
                Serial.println("Collision with a pterodactyl");
                return true;
              }
            } else {
              // For ground level pterodactyl, treat like a cactus
              if (dinoY + dinoCurrentHeight > obstacles[i].y) {
                Serial.println("Collision with a pterodactyl");
                return true;
              }
            }
        }
        else {
          // For cactuses, check normal collision
          if (dinoY + dinoCurrentHeight > GROUND_HEIGHT - obstacleHeight) {
            Serial.println("Collision with a cactus");
            return true;
          }
        }
      }
    }
  }
  return false;
}

// Display current time on TM1637
void displayCurrentTime() {
  timeDisplay.showNumberDecEx(currentGameTime, 0b01000000, true); // show with center colon
}

// Display highest time on TM1637
void displayHighestTime() {
  timeDisplay.showNumberDecEx(highestGameTime, 0b01000000, true); // show with center colon
}

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}

// Play a milestone sound
void playMilestoneSound() {
  playTone(523, 50);
  delay(50);
  playTone(659, 50);
  delay(50);
  playTone(784, 100);
  delay(100);
}
