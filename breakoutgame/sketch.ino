#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1637Display.h> // TM1637Display kütüphanesi eklendi

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int blackButtonPin = 3;  // Siyah düğme için pin tanımlaması
const int redButtonPin = 2;    // Kırmızı düğme için pin tanımlaması
const int potPin = A2;         // Potansiyometre için analog pin tanımlaması
int paddleX = SCREEN_WIDTH / 2; // Paddle'ın başlangıç X konumu

const int ballRadius = 3;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballSpeedX = 2; // Topun X eksenindeki hızı
int ballSpeedY = 2; // Topun Y eksenindeki hızı

bool gameStarted = false;
int lives = 3; // Can sayısı

// Can göstergesi LED'leri
const int ledPin1 = 4;
const int ledPin2 = 5;
const int ledPin3 = 6;

// TM1637Display için pinler
#define CLK 9
#define DIO 10
TM1637Display sevSeg(CLK, DIO); // TM1637Display nesnesi oluşturuldu

// Tuğla özellikleri
const int brickWidth = 20;
const int brickHeight = 8;
const int brickRowCount = 4;
const int brickColumnCount = SCREEN_WIDTH / brickWidth;
bool bricks[brickColumnCount][brickRowCount];

// Ödül özellikleri
const int rewardWidth = 5;
const int rewardHeight = 5;
bool rewardActive = false;
int rewardX, rewardY;

#define LEVEL_COUNT 2  // Toplam seviye sayısı
int currentLevel = 1; // Başlangıç seviyesi

void setup() {
  Serial.begin(9600);

  pinMode(blackButtonPin, INPUT_PULLUP);
  pinMode(redButtonPin, INPUT_PULLUP);
  pinMode(potPin, INPUT);

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);

  sevSeg.setBrightness(0x0a); // Parlaklık seviyesini ayarla

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // OLED ekran başlatma
  display.display();
  delay(2000);
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Oyun Giris Ekrani");
  display.println("Siyah tus basildiginda");
  display.println("oyun baslayacak");
  display.println("Kirmizi tus basildiginda");
  display.println("cikis yapilacak");
  display.display();
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
}

void loop() {
  if (!gameStarted) {
    if (digitalRead(blackButtonPin) == LOW) {
      startGame(); // Oyunu başlat
    }
    if (digitalRead(redButtonPin) == LOW) {
      exitGame(); // Oyundan çık
    }
  } else {
    if (digitalRead(redButtonPin) == LOW) {
      exitGame(); // Oyundan çık
    }

    // Potansiyometre değerini oku
    int potValue = analogRead(potPin);
    
    // Potansiyometre değerini ekran genişliğine orantıla ve paddle'ın X konumunu güncelle
    paddleX = map(potValue, 0, 1023, 0, SCREEN_WIDTH - 10); // -10, paddle'ın tam ortada olmasını sağlar

    // Topun hareketini güncelle
    updateBall();

    // Ekrana paddle'ı, topu ve tuğlaları çiz
    display.clearDisplay();
    display.fillRect(paddleX, SCREEN_HEIGHT - 5, 20, 5, SSD1306_WHITE); // Paddle boyutu ve rengi
    display.fillCircle(ballX, ballY, ballRadius, SSD1306_WHITE); // Top boyutu ve rengi
    drawBricks();
    if (rewardActive) {
      display.fillRect(rewardX, rewardY, rewardWidth, rewardHeight, SSD1306_WHITE);
    }
    display.display();
    delay(10);
  }
}

void startGame() {
  gameStarted = true;
  lives = 3; // Can sayısını sıfırla
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Oyun Basladi!");
  display.display();
  // 3 LED'i de yak
  digitalWrite(ledPin1, HIGH);
  digitalWrite(ledPin2, HIGH);
  digitalWrite(ledPin3, HIGH);

  // Tuğlaları başlat
  initBricks();
  // Ödülü başlat
  spawnReward();

  // Can göstergesi için başlangıç değerini göster
  sevSeg.showNumberDec(lives, false);
}

void exitGame() {
  gameStarted = false;
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Cikis Yapildi!");
  display.display();
  delay(1000);
  while (digitalRead(redButtonPin) == LOW) {}  // Kırmızı düğmeye basılı tutana kadar bekle
  display.clearDisplay();
  display.display();
 
}

int brickCollisionCounter = 0; // Tuğlara çarpma sayacı

void updateBall() {
  bool bricksExist = false;
  for (int i = 0; i < brickColumnCount; i++) {
    for (int j = 0; j < brickRowCount; j++) {
      if (bricks[i][j]) {
        bricksExist = true;
        break;
      }
    }
    if (bricksExist) {
      break;
    }
  }
  // Topun X konumunu güncelle
  ballX += ballSpeedX;

  // Topun Y konumunu güncelle
  ballY += ballSpeedY;

  // Topun duvarlara çarpmasını kontrol et
  if (ballX + ballRadius > SCREEN_WIDTH || ballX - ballRadius < 0) {
    ballSpeedX = -ballSpeedX; // Topun yönünü tersine çevir
  }
  if (ballY - ballRadius < 0) {
    ballSpeedY = -ballSpeedY; // Topun yönünü tersine çevir
  }

  // Topun paddle ile çarpışmasını kontrol et
  if (ballY + ballRadius >= SCREEN_HEIGHT - 5 && ballX >= paddleX && ballX <= paddleX + 20) {
    ballSpeedY = -ballSpeedY; // Topun yönünü tersine çevir
  }

  // Topun alt kenara çarpmasını kontrol et
  if (ballY + ballRadius >= SCREEN_HEIGHT) {
    ballX = SCREEN_WIDTH / 2; // Topu ekranın ortasına yerleştir
    ballY = SCREEN_HEIGHT / 2; // Topu ekranın ortasına yerleştir
    lives--; // Can sayısını azalt
    updateLifeIndicators(); // Can göstergesini güncelle
    if (lives <= 0) {
      gameOver(); // Canlar tükenirse oyunu sonlandır
    }
  }
 
  // Topun tuğlalara çarpışmasını kontrol et
  int ballColumn = ballX / brickWidth;
  int ballRow = ballY / brickHeight;
  if (ballRow >= 0 && ballRow < brickRowCount && ballColumn >= 0 && ballColumn < brickColumnCount && bricks[ballColumn][ballRow]) {
    bricks[ballColumn][ballRow] = false; // Tuğlayı kır
    
    ballSpeedY = -ballSpeedY; // Topun yönünü tersine çevir
    brickCollisionCounter++; // Tuğlara çarpma sayacını artır
    if (random(0, 10) == 0) { // %10 ihtimalle ödülü etkinleştir
      spawnReward();
    }
  }
  // Tüm tuğlalar kırıldıysa bir sonraki seviyeye geç
  if (!bricksExist) {
    
    currentLevel++;
    if (currentLevel > LEVEL_COUNT) {
      gameOver(); 
    } else {
      // Seviye değişikliklerini yap
      increaseLevelDifficulty();
      // Yeni seviyeye geçildiğini ekrana yazdır
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Seviye " + String(currentLevel) + "!");
      display.display();
      delay(2000);
      ballX = SCREEN_WIDTH / 2;
      ballY = SCREEN_HEIGHT / 2;
      initBricks(); // Yeni seviyeye ait tuğlaları başlat
      spawnReward(); // Yeni seviyeye ait ödülü başlat
    }
  }

  // Ödülün hareketini güncelle
  if (rewardActive) {
    rewardY++;
    if (rewardY >= SCREEN_HEIGHT) {
      rewardActive = false; // Ödül ekranın altına düştüğünde ödülü devre dışı bırak
    }
    // Ödülün paddle ile çarpışmasını kontrol et
    if (rewardY + rewardHeight >= SCREEN_HEIGHT - 5 && rewardX >= paddleX && rewardX <= paddleX + 20) {
      if(lives<3)
      {rewardActive = false; // Ödülü devre dışı bırak
     
      lives++; // Can sayısını artır
      updateLifeIndicators(); // Can göstergesini güncelle
      }
    }
  }

   // Tuğlara çarpma sayacını yedi segmente yazdır
  sevSeg.showNumberDec(brickCollisionCounter);
}

void updateLifeIndicators() {
  // Can göstergesi LED'lerini güncelle
  digitalWrite(ledPin1, lives >= 1 ? HIGH : LOW);
  digitalWrite(ledPin2, lives >= 2 ? HIGH : LOW);
  digitalWrite(ledPin3, lives >= 3 ? HIGH : LOW);

  // Yedi segmenti güncelle
  sevSeg.showNumberDec(lives, false);
}

void gameOver() {
  gameStarted = false;
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Oyun Bitti!");
  display.println("Puan: " + String(brickCollisionCounter)); // Tuğlara çarpma sayısını ekrana yazdır
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
  
}

void initBricks() {
  // Tuğlaları başlat
  for (int i = 0; i < brickColumnCount; i++) {
    for (int j = 0; j < brickRowCount / 2; j++) { // Tuğla sayısını yarıya düşür
      bricks[i][j] = true;
    }
  }
}

void drawBricks() {
  // Tuğlaları çiz
  for (int i = 0; i < brickColumnCount; i++) {
    for (int j = 0; j < brickRowCount; j++) {
      if (bricks[i][j]) {
        display.fillRect(i * brickWidth, j * brickHeight, brickWidth, brickHeight, SSD1306_WHITE);
        // Kenarları kalın çiz
        display.drawFastHLine(i * brickWidth, j * brickHeight, brickWidth, SSD1306_BLACK);
        display.drawFastHLine(i * brickWidth, j * brickHeight + brickHeight - 1, brickWidth, SSD1306_BLACK);
        display.drawFastVLine(i * brickWidth, j * brickHeight, brickHeight, SSD1306_BLACK);
        display.drawFastVLine(i * brickWidth + brickWidth - 1, j * brickHeight, brickHeight, SSD1306_BLACK);
      }
    }
  }
}

void spawnReward() {
  // Ödülü rastgele bir konuma yerleştir
  rewardX = random(0, SCREEN_WIDTH - rewardWidth);
  rewardY = 0;
  rewardActive = true;
}

void increaseLevelDifficulty() {
  // Seviye ayarlarını güncelle
  switch (currentLevel) {
    case 2:
      // Topun hızını iki katına çıkar
      ballSpeedX *= 1.3;
      ballSpeedY *= 1.3;
      // Tuğla sayısını 2 katına çıkar
      int newBrickRowCount = brickRowCount * 1.5; // Yeni tuğla satır sayısı
      for (int i = 0; i < brickColumnCount; i++) {
        for (int j = 0; j < newBrickRowCount; j++) {
          // Örnek bir farklı tuğla deseni
          if (i % 2 == 0 && j % 3 == 0) {
            bricks[i][j] = true;
          } else {
            bricks[i][j] = false;
          }
        }
      }
      break;
    
  }
}