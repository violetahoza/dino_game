#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUTTON_PIN 2
#define BUZZER_PIN 3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Game states
enum GameState {
  START_SCREEN,
  PLAYING,
  GAME_OVER
};

GameState gameState = START_SCREEN;

// Game constants
const int DINO_WIDTH = 25;
const int DINO_HEIGHT = 26;
const int GROUND_HEIGHT = 54;
const int CACTUS_WIDTH = 12;
const int CACTUS_HEIGHT = 24;

// Game variables
int dinoY;
float dinoVelocity;
bool isJumping = false;
int score = 0;
int highestScore = 0;

// Background elements
struct Cloud {
  int x;
  int y;
  bool active;
};

struct Star {
  int x;
  int y;
  bool active;
  unsigned long lastBlink;
  bool visible;
};

const int MAX_CLOUDS = 3;
const int MAX_STARS = 8;
Cloud clouds[MAX_CLOUDS];
Star stars[MAX_STARS];

// Obstacle management
const int MAX_OBSTACLES = 3;
int obstacleX[MAX_OBSTACLES];
bool obstacleActive[MAX_OBSTACLES];

// Bitmap definitions
const unsigned char PROGMEM dino[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x06, 0xff, 0x00, 0x00, 0x0e, 0xff, 0x00, 
  0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
  0x00, 0x0f, 0xfc, 0x00, 0x40, 0x0f, 0xc0, 0x00, 0x40, 0x1f, 0x80, 0x00, 0x40, 0x7f, 0x80, 0x00, 
  0x60, 0xff, 0xe0, 0x00, 0x71, 0xff, 0xa0, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0x00, 
  0x7f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 
  0x03, 0xfc, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 
  0x01, 0x0c, 0x00, 0x00, 0x01, 0x8e, 0x00, 0x00
};

const unsigned char PROGMEM cactus[] = {
  0x1e, 0x00, 0x1f, 0x00, 0x1f, 0x40, 0x1f, 0xe0, 0x1f, 0xe0, 0xdf, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 
  0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xc0, 0xff, 0x00, 0xff, 0x00, 0x7f, 0x00, 
  0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00
};

const unsigned char PROGMEM cloud[] = {
    0x0F, 0xC0, 0x1F, 0xE0, 0x3F, 0xF0, 0x3F, 0xF0, 0x1F, 0xE0, 0x0F, 0xC0
};

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();
  
  randomSeed(analogRead(0));
  initializeGame();
}

void initializeGame() {
  dinoY = GROUND_HEIGHT - DINO_HEIGHT;
  dinoVelocity = 0;
  isJumping = false;
  score = 0;
  
  // Initialize clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    clouds[i].active = false;
    clouds[i].x = SCREEN_WIDTH + (i * SCREEN_WIDTH/2);
    clouds[i].y = random(0, GROUND_HEIGHT - 30);
  }
  
  // Initialize stars
  for(int i = 0; i < MAX_STARS; i++) {
    stars[i].active = true;
    stars[i].x = random(0, SCREEN_WIDTH);
    stars[i].y = random(0, GROUND_HEIGHT - 40);
    stars[i].lastBlink = millis();
    stars[i].visible = true;
  }
  
  // Initialize obstacles
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    obstacleActive[i] = false;
    obstacleX[i] = SCREEN_WIDTH + (i * SCREEN_WIDTH/2);
  }
}

void drawStartScreen() {
  display.clearDisplay();
  
  // Draw title
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 10);
  display.print("DINO RUN");
  
  // Draw instructions
  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print("Push button to play");
  
  // Draw highest score
  display.setCursor(10, 50);
  display.print("Highest Score: ");
  display.print(highestScore);
  
  display.display();
}

void updateBackground() {
  // Update clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    if (clouds[i].active) {
      clouds[i].x -= 1;
      if (clouds[i].x < -20) {
        clouds[i].active = false;
      }
    } else if (random(100) < 1) {
      clouds[i].active = true;
      clouds[i].x = SCREEN_WIDTH;
      clouds[i].y = random(0, GROUND_HEIGHT - 30);
    }
  }
  
  // Update stars (blinking effect)
  unsigned long currentTime = millis();
  for(int i = 0; i < MAX_STARS; i++) {
    if (currentTime - stars[i].lastBlink > 1000 + random(2000)) {
      stars[i].visible = !stars[i].visible;
      stars[i].lastBlink = currentTime;
    }
  }
}

void drawBackground() {
  // Draw stars
  for(int i = 0; i < MAX_STARS; i++) {
    if (stars[i].active && stars[i].visible) {
      display.drawPixel(stars[i].x, stars[i].y, WHITE);
    }
  }
  
  // Draw clouds
  for(int i = 0; i < MAX_CLOUDS; i++) {
    if (clouds[i].active) {
      display.drawBitmap(clouds[i].x, clouds[i].y, cloud, 16, 6, WHITE);
    }
  }
}

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}

void updateGame() {
  // Jump mechanics
  if (!isJumping && digitalRead(BUTTON_PIN) == LOW) {
    isJumping = true;
    dinoVelocity = -8.0;
    playTone(800, 50);
  }
  
  if (isJumping) {
    dinoY += dinoVelocity;
    dinoVelocity += 0.6;
    
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

bool checkCollision() {
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      if (obstacleX[i] < (DINO_WIDTH) &&
          (obstacleX[i] + CACTUS_WIDTH) > 0 &&
          (dinoY + DINO_HEIGHT) > (GROUND_HEIGHT - CACTUS_HEIGHT)) {
        return true;
      }
    }
  }
  return false;
}

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
    for(int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacleActive[i]) {
        display.drawBitmap(obstacleX[i], GROUND_HEIGHT - CACTUS_HEIGHT, 
                          cactus, CACTUS_WIDTH, CACTUS_HEIGHT, WHITE);
      }
    }
    
    // Draw score during gameplay
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(SCREEN_WIDTH - 40, 0);
    display.print(score);
  }
  
  display.display();
}

void loop() {
  switch (gameState) {
    case START_SCREEN:
      drawStartScreen();
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(200);  // Debounce
        gameState = PLAYING;
        initializeGame();
      }
      break;
      
    case PLAYING:
      updateGame();
      drawGameOver();
      break;
      
    case GAME_OVER:
      drawGameOver();
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(200);  // Debounce
        gameState = START_SCREEN;
      }
      break;
  }
  
  delay(16);  // Approximately 60 FPS
}