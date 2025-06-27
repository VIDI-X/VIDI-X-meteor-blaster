#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <FastLED.h>

// ------------------------- EKRAN I DEFINICIJE -------------------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
public:
  LGFX(void) {
    auto cfg = _bus_instance.config();
    cfg.spi_host = VSPI_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 40000000;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = true;
    cfg.use_lock = true;
    cfg.dma_channel = 1;
    cfg.pin_sclk = 18;
    cfg.pin_mosi = 23;
    cfg.pin_miso = 19;
    cfg.pin_dc = 21;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto pcfg = _panel_instance.config();
    pcfg.pin_cs = 5;
    pcfg.pin_rst = -1;
    pcfg.pin_busy = -1;
    pcfg.memory_width = 240;
    pcfg.memory_height = 320;
    pcfg.panel_width = 240;
    pcfg.panel_height = 320;
    pcfg.offset_rotation = 1;
    pcfg.invert = false;
    pcfg.rgb_order = false;
    pcfg.dlen_16bit = false;
    pcfg.bus_shared = true;
    _panel_instance.config(pcfg);
    setPanel(&_panel_instance);
  }
};

static LGFX lcd;
static LGFX_Sprite canvas(&lcd);

#define LED_PIN 26
#define NUM_LEDS 5
CRGB leds[NUM_LEDS];

#define SPEAKER_PIN 25
#define LEDC_CHANNEL 0
#define LEDC_RESOLUTION 8
#define LEDC_BASE_FREQ 2000

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define UI_HEIGHT 35
#define PLAYER_SIZE 24
#define MAX_METEORS 16
#define MAX_CELLS 3

#define BTN_LR 34
#define BTN_UD 35
#define BTN_A 32
#define BTN_RESTART 0

#define MOVE_STEP 8

#define BULLET_SPEED 14
#define BULLET_MAX 8

#define COLOR_UI_BG 0x18E3
#define COLOR_TEXT 0xFFFF

// ------------------------- STRUKTURE -------------------------
struct Meteor {
  float x, y, dx, dy, speed, angle, rotationSpeed;
  int size, impact;
  bool active, isSmall;
  uint8_t points;
  float offsets[16];
  uint16_t color;
};

struct Bullet {
  float x, y, dx, dy;
  bool active;
};

struct EnergyCell {
  float x, y;
  bool collected;
};

struct Player {
  float x, y;
  int health;
  int energy;
  int score;
};

Player player;
Meteor meteors[MAX_METEORS];
EnergyCell cells[MAX_CELLS];
Bullet bullets[BULLET_MAX];

unsigned long lastSpawn = 0;
bool gameOver = false;

bool cannonFired = false;
unsigned long lastCannonFlash = 0;
#define CANNON_FLASH_MS 70

int level = 3;
int avoidedMeteors = 0;
int collectedPoints = 0;
int destroyedMeteors = 0;
bool quizMode = false;
bool finalQuizMode = false;


void checkLevelProgression() {
  if (level == 1 && avoidedMeteors >= 20 && !quizMode) {
    quizMode = true;
  }
  if (level == 2 && player.energy >= 250 && !quizMode) {
    quizMode = true;
  }
  if (level == 3 && player.score >= 1000 && destroyedMeteors >= 10 && !quizMode) {
    quizMode = true;
  }
}

void handleMeteorOutOfScreen(Meteor &m) {
  if (m.y > SCREEN_HEIGHT + 50) {
    m.active = false;
    if (level == 1) avoidedMeteors++;
  }
}

void fireCannon() {
  cannonFired = true;
  lastCannonFlash = millis();

  // LED efekt – žuti blink
  fill_solid(leds, NUM_LEDS, CRGB::Yellow);
  FastLED.show();
  delay(30);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  for (int i = 0; i < BULLET_MAX; i++) {
    if (!bullets[i].active) {
      bullets[i].x = player.x;
      bullets[i].y = player.y - PLAYER_SIZE / 2 - 2;
      bullets[i].dx = 0;
      bullets[i].dy = -BULLET_SPEED;
      bullets[i].active = true;
      cannonFired = true;
      lastCannonFlash = millis();
      break;
    }
  }
}

void splitMeteor(Meteor &m) {
  m.active = false;
  int smallCount = random(3, 5);
  int spawned = 0;
  for (int i = 0; i < MAX_METEORS && spawned < smallCount; i++) {
    if (!meteors[i].active) {
      meteors[i] = m;
      meteors[i].active = true;
      meteors[i].size = m.size / 2 + random(-3, 3);
      meteors[i].isSmall = true;
      meteors[i].impact = meteors[i].size / 5 + 1;
      float angle = random(0, 360) * 0.01745;
      float spd = random(3, 7);
      meteors[i].dx = cos(angle) * spd;
      meteors[i].dy = sin(angle) * spd;
      meteors[i].angle += random(-30, 30) * 0.01745;
      meteors[i].rotationSpeed = m.rotationSpeed * (0.7 + random(-10, 10) * 0.01);
      meteors[i].color += random(-300, 300);
      spawned++;
    }
  }
  destroyedMeteors++;
}

void checkMeteorCollisions() {
  for (int i = 0; i < MAX_METEORS; i++) {
    if (!meteors[i].active) continue;
    for (int j = i + 1; j < MAX_METEORS; j++) {
      if (!meteors[j].active) continue;
      float dx = meteors[i].x - meteors[j].x;
      float dy = meteors[i].y - meteors[j].y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < (meteors[i].size + meteors[j].size) / 2) {
        float temp_dx = meteors[i].dx;
        float temp_dy = meteors[i].dy;
        meteors[i].dx = meteors[j].dx;
        meteors[i].dy = meteors[j].dy;
        meteors[j].dx = temp_dx;
        meteors[j].dy = temp_dy;
      }
    }
  }
}

void checkPlayerCollision() {
  for (int i = 0; i < MAX_METEORS; i++) {
    if (!meteors[i].active) continue;
    float dx = player.x - meteors[i].x;
    float dy = player.y - meteors[i].y;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < meteors[i].size / 2 + PLAYER_SIZE / 2 - 2) {
      player.health -= meteors[i].impact;
      meteors[i].active = false;
      playCollisionSound();                   // <-- ZVUK
      fill_solid(leds, NUM_LEDS, CRGB::Red);  // <-- LED efekt
      FastLED.show();
      delay(50);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
  }
}

void updateGame() {
  int analogLR = analogRead(BTN_LR);
  int analogUD = analogRead(BTN_UD);

  if (analogLR > 4000) player.x -= MOVE_STEP;
  if (analogLR > 1800 && analogLR < 2200) player.x += MOVE_STEP;

  if (level >= 2) {
    if (analogUD > 4000) player.y -= MOVE_STEP;
    if (analogUD > 1800 && analogUD < 2200) player.y += MOVE_STEP;
  }

  if (player.x < PLAYER_SIZE / 2) player.x = PLAYER_SIZE / 2;
  if (player.x > SCREEN_WIDTH - PLAYER_SIZE / 2) player.x = SCREEN_WIDTH - PLAYER_SIZE / 2;
  if (player.y < UI_HEIGHT + PLAYER_SIZE / 2) player.y = UI_HEIGHT + PLAYER_SIZE / 2;
  if (player.y > SCREEN_HEIGHT - PLAYER_SIZE / 2) player.y = SCREEN_HEIGHT - PLAYER_SIZE / 2;

  if (millis() - lastSpawn > 1100) {
    for (int i = 0; i < MAX_METEORS; i++) {
      if (!meteors[i].active) {
        meteors[i].x = random(40, SCREEN_WIDTH - 40);
        meteors[i].y = -30;
        meteors[i].speed = random(2, 5);
        meteors[i].size = random(36, 48);
        meteors[i].active = true;
        meteors[i].angle = random(0, 360) * 0.01745;
        meteors[i].rotationSpeed = (random(-8, 9)) * 0.008;
        meteors[i].points = random(8, 13);
        for (uint8_t j = 0; j < meteors[i].points; j++)
          meteors[i].offsets[j] = meteors[i].size / 2 * (0.7 + random(0, 31) * 0.01);
        meteors[i].color = 0x7BEF + random(-500, 500);
        meteors[i].dx = 0;
        meteors[i].dy = meteors[i].speed;
        meteors[i].isSmall = false;
        meteors[i].impact = meteors[i].size / 4 + 3;
        break;
      }
    }
    if (level >= 2 && random(100) < 30) {
      for (int i = 0; i < MAX_CELLS; i++) {
        if (cells[i].collected) {
          cells[i].x = random(50, SCREEN_WIDTH - 50);
          cells[i].y = random(100, SCREEN_HEIGHT - 100);
          cells[i].collected = false;
          break;
        }
      }
    }
    lastSpawn = millis();
  }

  for (int i = 0; i < MAX_METEORS; i++) {
    if (meteors[i].active) {
      meteors[i].x += meteors[i].dx;
      meteors[i].y += meteors[i].dy;
      meteors[i].angle += meteors[i].rotationSpeed;
      if (meteors[i].y > SCREEN_HEIGHT + 50) {
        meteors[i].active = false;
        avoidedMeteors++;
      }
    }
  }

  for (int i = 0; i < MAX_CELLS; i++) {
    if (!cells[i].collected) {
      float dx = player.x - cells[i].x;
      float dy = player.y - cells[i].y;
      if (sqrt(dx * dx + dy * dy) < 28) {
        player.energy += 25;
        player.score += 100;
        collectedPoints++;
        cells[i].collected = true;
        playCollectSound();  // <-- DODAJ OVDJE
        // LED efekt
        fill_solid(leds, NUM_LEDS, CRGB::Green);
        FastLED.show();
        delay(50);
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
      }
    }
  }

  for (int b = 0; b < BULLET_MAX; b++) {
    if (bullets[b].active) {
      bullets[b].x += bullets[b].dx;
      bullets[b].y += bullets[b].dy;
      if (bullets[b].y < 0) bullets[b].active = false;
    }
  }

  for (int b = 0; b < BULLET_MAX; b++) {
    if (!bullets[b].active) continue;
    for (int m = 0; m < MAX_METEORS; m++) {
      if (!meteors[m].active) continue;
      float dx = bullets[b].x - meteors[m].x;
      float dy = bullets[b].y - meteors[m].y;
      if (sqrt(dx * dx + dy * dy) < meteors[m].size / 2) {
        bullets[b].active = false;
        if (!meteors[m].isSmall && meteors[m].size > 20) {
          splitMeteor(meteors[m]);
        } else {
          meteors[m].active = false;
          destroyedMeteors++;
        }
        player.score += meteors[m].isSmall ? 30 : 100;
        break;
      }
    }
  }

  if (player.health <= 0) gameOver = true;
  checkMeteorCollisions();
  updateMeteorPhysics();
  checkPlayerCollision();
}

void drawGame() {
  canvas.fillScreen(0x0000);
  canvas.fillRect(0, 0, SCREEN_WIDTH, UI_HEIGHT, COLOR_UI_BG);
  for (int i = 0; i < MAX_METEORS; i++) {
    if (!meteors[i].active) continue;
    drawMeteor(meteors[i]);
  }
  for (int i = 0; i < MAX_CELLS; i++) {
    if (!cells[i].collected) {
      canvas.fillCircle(cells[i].x, cells[i].y, 11, 0x07E0);
      canvas.drawCircle(cells[i].x, cells[i].y, 14, 0xFFFF);
    }
  }
  drawPlayerShip(player.x, player.y);
  for (int b = 0; b < BULLET_MAX; b++) {
    if (bullets[b].active) {
      canvas.fillCircle(bullets[b].x, bullets[b].y, 3, 0xFFE0);
    }
  }
  canvas.setTextColor(COLOR_TEXT);
  canvas.setTextSize(1);
  canvas.setCursor(10, 10);
  canvas.printf("HP: %d", player.health);
  canvas.setCursor(SCREEN_WIDTH / 2 - 30, 10);
  canvas.printf("Score: %d", player.score);
  canvas.setCursor(SCREEN_WIDTH - 80, 10);
  canvas.printf("E: %d", player.energy);
  canvas.pushSprite(0, 0);
}

void showGameOver() {
  canvas.fillScreen(0x0000);
  canvas.setTextSize(3);
  canvas.setCursor(60, 60);
  canvas.print("GAME OVER");

  canvas.pushSprite(0, 0);

  fill_gradient_RGB(leds, NUM_LEDS, CRGB::Red, CRGB::Black);
  FastLED.show();

  if (digitalRead(BTN_RESTART) == LOW) {
    delay(300);
    ESP.restart();
  }
}


// --- Meteoriti ---
void generateMeteor() {
  for (int i = 0; i < MAX_METEORS; i++) {
    if (!meteors[i].active) {
      meteors[i].x = random(40, SCREEN_WIDTH - 40);
      meteors[i].y = -30;
      meteors[i].speed = random(2, 5);
      meteors[i].size = random(36, 48);
      meteors[i].active = true;
      meteors[i].angle = random(0, 360) * 0.01745;
      meteors[i].rotationSpeed = (random(-8, 9)) * 0.008;
      meteors[i].points = random(8, 13);
      for (uint8_t j = 0; j < meteors[i].points; j++)
        meteors[i].offsets[j] = meteors[i].size / 2 * (0.7 + random(0, 31) * 0.01);
      int tint = random(3);
      if (tint == 0)
        meteors[i].color = 0x7BEF + random(-500, 500);
      else if (tint == 1)
        meteors[i].color = 0x6DEF + random(-600, 300);
      else
        meteors[i].color = 0xFEA0 + random(-400, 200);
      meteors[i].dx = 0;
      meteors[i].dy = meteors[i].speed;
      meteors[i].isSmall = false;
      meteors[i].impact = meteors[i].size / 4 + 3;
      break;
    }
  }
}

// --- Energijske ćelije ---
void generateEnergyCell() {
  for (int i = 0; i < MAX_CELLS; i++) {
    if (cells[i].collected) {
      cells[i].x = random(50, SCREEN_WIDTH - 50);
      cells[i].y = random(100, SCREEN_HEIGHT - 100);
      cells[i].collected = false;
      break;
    }
  }
}

void updateMeteorPhysics() {
  for (int i = 0; i < MAX_METEORS; i++) {
    if (!meteors[i].active) continue;

    meteors[i].x += meteors[i].dx;
    meteors[i].y += meteors[i].dy;
    meteors[i].angle += meteors[i].rotationSpeed;

    if (meteors[i].x < meteors[i].size / 2 || meteors[i].x > SCREEN_WIDTH - meteors[i].size / 2)
      meteors[i].dx = -meteors[i].dx;

    if (meteors[i].x < -50 || meteors[i].x > SCREEN_WIDTH + 50 || meteors[i].y < -50 || meteors[i].y > SCREEN_HEIGHT + 50)
      meteors[i].active = false;

    for (int j = i + 1; j < MAX_METEORS; j++) {
      if (!meteors[j].active) continue;
      float dx = meteors[j].x - meteors[i].x;
      float dy = meteors[j].y - meteors[i].y;
      float dist = sqrt(dx * dx + dy * dy);
      float minDist = meteors[i].size / 2 + meteors[j].size / 2;
      if (dist < minDist && dist > 1) {
        float nx = dx / dist, ny = dy / dist;
        float dvx = meteors[i].dx - meteors[j].dx;
        float dvy = meteors[i].dy - meteors[j].dy;
        float impact = dvx * nx + dvy * ny;
        if (impact < 0) {
          float force = (meteors[i].impact + meteors[j].impact) / 2.0;
          meteors[i].dx -= nx * force / meteors[i].impact;
          meteors[i].dy -= ny * force / meteors[i].impact;
          meteors[j].dx += nx * force / meteors[j].impact;
          meteors[j].dy += ny * force / meteors[j].impact;
        }
      }
    }
  }
}

// --- Kretanje igrača s analogRead (VIDI X joystick stil) ---
void updatePlayerMovement() {
  int analogLR = analogRead(BTN_LR);
  int analogUD = analogRead(BTN_UD);

  // Lijevo
  if (analogLR > 4000) player.x -= MOVE_STEP;
  // Desno
  if (analogLR > 1800 && analogLR < 2200) player.x += MOVE_STEP;
  // Gore
  if (analogUD > 4000) player.y -= MOVE_STEP;
  // Dolje
  if (analogUD > 1800 && analogUD < 2200) player.y += MOVE_STEP;

  // Clamp
  if (player.x < PLAYER_SIZE / 2) player.x = PLAYER_SIZE / 2;
  if (player.x > SCREEN_WIDTH - PLAYER_SIZE / 2) player.x = SCREEN_WIDTH - PLAYER_SIZE / 2;
  if (player.y < UI_HEIGHT + PLAYER_SIZE / 2) player.y = UI_HEIGHT + PLAYER_SIZE / 2;
  if (player.y > SCREEN_HEIGHT - PLAYER_SIZE / 2) player.y = SCREEN_HEIGHT - PLAYER_SIZE / 2;
}

// --- Update metka ---
void updateBullets() {
  for (int i = 0; i < BULLET_MAX; i++) {
    if (bullets[i].active) {
      bullets[i].x += bullets[i].dx;
      bullets[i].y += bullets[i].dy;
      if (bullets[i].y < 0) bullets[i].active = false;
    }
  }
}

void drawBullets() {
  for (int i = 0; i < BULLET_MAX; i++) {
    if (bullets[i].active) {
      canvas.fillCircle(bullets[i].x, bullets[i].y, 3, 0xFFE0);
      canvas.drawCircle(bullets[i].x, bullets[i].y, 3, 0xFFFF);
    }
  }
}

// --- Crtanje meteora ---
void drawMeteor(Meteor &m) {
  float cx = m.x, cy = m.y;
  uint8_t pts = m.points;
  float a = m.angle;
  int16_t px[16], py[16];
  for (uint8_t j = 0; j < pts; j++) {
    float theta = a + j * (2 * PI / pts);
    float r = m.offsets[j];
    px[j] = cx + cos(theta) * r;
    py[j] = cy + sin(theta) * r;
  }
  for (uint8_t j = 0; j < pts; j++) {
    uint8_t next = (j + 1) % pts;
    canvas.fillTriangle(cx, cy, px[j], py[j], px[next], py[next], m.color);
    canvas.drawLine(px[j], py[j], px[next], py[next], 0xFFFF);
  }
}

// --- Player ship, refleksije, sjaj, topovi ---
void drawPlayerShip(float x, float y) {
  int shipW = PLAYER_SIZE;
  int shipH = PLAYER_SIZE + 8;
  int canopyH = 10;
  int motorW = 6;
  int motorH = 14;
  int fireLen = 16;

  // Tijelo broda
  canvas.fillTriangle(
    x - shipW / 2, y + shipH / 2,
    x + shipW / 2, y + shipH / 2,
    x, y - shipH / 2 + 4,
    0xC618);
  canvas.drawTriangle(
    x - shipW / 2, y + shipH / 2,
    x + shipW / 2, y + shipH / 2,
    x, y - shipH / 2 + 4,
    0x528A);

  // Kupola + refleksija
  canvas.fillEllipse(x, y - shipH / 4, shipW / 4, canopyH / 2, 0xA145);  // Smeđa
  canvas.drawEllipse(x, y - shipH / 4, shipW / 4, canopyH / 2, 0x630C);
  canvas.fillEllipse(x, y - shipH / 4, shipW / 7, canopyH / 4, 0x033F);              // plavi sjaj
  canvas.drawLine(x - shipW / 12, y - shipH / 4 - 1, x, y - shipH / 4 - 3, 0xFFFF);  // window reflection

  // Motori
  int mx1 = x - shipW / 2 + motorW / 2;
  int mx2 = x + shipW / 2 - motorW / 2;
  int my = y + shipH / 2 - motorH / 2;
  canvas.fillRect(mx1 - motorW / 2, my, motorW, motorH, 0x8410);
  canvas.fillRect(mx2 - motorW / 2, my, motorW, motorH, 0x8410);
  canvas.drawRect(mx1 - motorW / 2, my, motorW, motorH, 0xFFFF);
  canvas.drawRect(mx2 - motorW / 2, my, motorW, motorH, 0xFFFF);

  // Vatra iz motora
  for (int f = 0; f < 3; f++) {
    int fireOff = random(-2, 3);
    uint16_t fireColor = (random(2) == 0) ? 0xF800 : 0xFFE0;
    canvas.drawLine(mx1, my + motorH, mx1 + fireOff, my + motorH + fireLen + random(4), fireColor);
    fireColor = (random(2) == 0) ? 0xF800 : 0xFFE0;
    canvas.drawLine(mx2, my + motorH, mx2 + fireOff, my + motorH + fireLen + random(4), fireColor);
  }

  // Topovi (treptanje kad puca)
  int noseY = y - shipH / 2 + 4;
  int noseX = x;
  uint16_t cannonColor = (cannonFired && millis() - lastCannonFlash < CANNON_FLASH_MS) ? 0xFFE0 : 0x528A;
  canvas.drawLine(noseX - 6, noseY + 4, noseX - 6, noseY - 8, cannonColor);
  canvas.drawLine(noseX + 6, noseY + 4, noseX + 6, noseY - 8, cannonColor);
  if (cannonFired && millis() - lastCannonFlash < CANNON_FLASH_MS) {
    canvas.drawPixel(noseX - 6, noseY - 8, 0xFFFF);
    canvas.drawPixel(noseX + 6, noseY - 8, 0xFFFF);
  }
}

// --- Zvuk ---
void playCollectSound() {
  ledcWriteTone(LEDC_CHANNEL, 1040);
  delay(36);
  ledcWriteTone(LEDC_CHANNEL, 0);
}

void playCollisionSound() {
  for (int i = 320; i >= 70; i -= 80) {
    ledcWriteTone(LEDC_CHANNEL, i);
    delay(18);
  }
  ledcWriteTone(LEDC_CHANNEL, 0);
}

// --- INIT ---
void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(0);
  canvas.setPsram(true);
  canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  ledcSetup(LEDC_CHANNEL, LEDC_BASE_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(SPEAKER_PIN, LEDC_CHANNEL);

  pinMode(BTN_LR, INPUT);
  pinMode(BTN_UD, INPUT);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_RESTART, INPUT_PULLUP);

  player.x = SCREEN_WIDTH / 2;
  player.y = SCREEN_HEIGHT - 50;
  player.health = 100;
  player.energy = 0;
  player.score = 0;

  for (int i = 0; i < MAX_METEORS; i++) meteors[i].active = false;
  for (int i = 0; i < MAX_CELLS; i++) cells[i].collected = true;
  for (int i = 0; i < BULLET_MAX; i++) bullets[i].active = false;
  gameOver = false;

  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    delay(100);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    //delay(100);
  }
}

void loop() {
    static bool lastFire = HIGH;
    bool nowFire = digitalRead(BTN_A);
    if (lastFire == HIGH && nowFire == LOW && level >= 3) {
      fireCannon();
    }
    lastFire = nowFire;
    if (!gameOver) {
      updateGame();
      drawGame();
      delay(16);
    } else {
      showGameOver();
    }
}
