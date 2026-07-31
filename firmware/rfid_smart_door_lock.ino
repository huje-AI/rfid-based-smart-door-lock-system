#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Keypad.h>

// Pin definitions
#define BUZZER_PIN A0
#define BUTTON_PIN A1
#define LED_PIN A2
#define RST_PIN A3
#define SS_PIN 10

// Keypad configuration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Initialize LCD and RFID
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Constants
const String MASTER_PASSWORD = "12345678";
const String UNLOCK_PASSWORD = "07122004";
const int MAX_CARDS = 10;
const int CARD_SIZE = 4;
const int MASTER_CARD_ADDR = 0;
const int GUEST_CARDS_ADDR = 10;

// State variables
int currentMode = 0;  // 0: normal, 1: add master, 2: add guest, 3: delete card, 4: delete all
int wrongAttempts = 0;
unsigned long lastActionTime = 0;
bool isAuthenticated = false;
bool isSystemLocked = false;

void setup() {
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Show welcome message
  displayWelcome();
}

void loop() {
  // Check if system is locked
  if (isSystemLocked) {
    if (millis() - lastActionTime >= 10000) {
      isSystemLocked = false;
      wrongAttempts = 0;
      displayWelcome();
    }
    return;
  }

  // Handle button press for mode changes
  if (isAuthenticated && digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      currentMode = (currentMode + 1) % 5;
      updateModeDisplay();
      lastActionTime = millis();
      while (digitalRead(BUTTON_PIN) == LOW);
    }
  }

  // Check for timeout
  if (currentMode != 0 && millis() - lastActionTime >= 10000) {
    currentMode = 0;
    displayWelcome();
  }

  // Handle RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleRFIDCard();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  // Handle keypad
  char key = keypad.getKey();
  if (key) {
    handleKeypad(key);
  }
}

void displayWelcome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Xin Chao!");
  lcd.setCursor(0, 1);
  lcd.print("Moi Quet The");
}

void handleRFIDCard() {
  // Nếu đã quét sai 5 lần, không cho quét thẻ nữa
  if (wrongAttempts >= 5) {
    lcd.clear();
    lcd.print("He Thong Khoa!");
    lcd.setCursor(0, 1);
    lcd.print("Nhap Mat Khau!");
    beepError();
    delay(2000);
    lcd.clear();
    lcd.print("Nhap Mat Khau:");
    return;
  }

  byte cardUID[4];
  for (byte i = 0; i < 4; i++) {
    cardUID[i] = mfrc522.uid.uidByte[i];
  }

  switch (currentMode) {
    case 0:
      if (checkCard(cardUID)) {
        successAccess();
        delay(2000);
        displayWelcome();
      } else {
        wrongAttempts++;
        failedAccess();
        delay(2000);
        if (wrongAttempts < 5) {
          displayWelcome();
        }
      }
      break;

    case 1:  // Thêm thẻ chính
      if (isAuthenticated) {
        if (checkCard(cardUID)) {
          lcd.clear();
          lcd.print("The Da Ton Tai!");
          beepError();
          delay(2000);
          displayWelcome();
        } else {
          saveMasterCard(cardUID);
          lcd.clear();
          lcd.print("Da Them The Chinh");
          beepSuccess();
          delay(2000);
          currentMode = 0;
          displayWelcome();
        }
      }
      break;

    case 2:  // Thêm thẻ phụ
      if (isAuthenticated) {
        if (checkCard(cardUID)) {
          lcd.clear();
          lcd.print("The Da Ton Tai!");
          beepError();
          delay(2000);
          displayWelcome();
        } else {
          if (saveGuestCard(cardUID)) {
            lcd.clear();
            lcd.print("Da Them The Phu");
            beepSuccess();
            delay(2000);
            currentMode = 0;
            displayWelcome();
          }
        }
      }
      break;

    case 3:
      if (isAuthenticated) {
        deleteCard(cardUID);
        delay(2000);
        currentMode = 0;
        displayWelcome();
      }
      break;

    case 4:
      if (checkMasterCard(cardUID)) {
        deleteAllGuestCards();
        currentMode = 0;  // Reset về chế độ bình thường
      }
      break;
  }
}

void beepKeypress() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50);  // Short beep duration
  digitalWrite(BUZZER_PIN, LOW);
}

void handleKeypad(char key) {
  static String input = "";
  static bool waitingForAuth = false;
  static int passwordAttempts = 0;  // Đếm số lần nhập sai mật khẩu
  const int MAX_PASSWORD_ATTEMPTS = 5;  // Số lần nhập tối đa
  
  beepKeypress();  // Add beep sound for any key press
  
  // Nếu đã quét sai 5 lần, cho phép nhập mật khẩu trực tiếp
  if (wrongAttempts >= 5 && !waitingForAuth && key != 'A' && key != '*' && key != '#') {
    waitingForAuth = true;
    input = key;  // Bắt đầu với phím vừa nhấn
    lcd.clear();
    lcd.print("Mat Khau:");
    lcd.setCursor(0, 1);
    lcd.print("*");
    return;
  }
  
  if (key == 'A' && !waitingForAuth && !isAuthenticated && wrongAttempts < 5) {
    lcd.clear();
    if (currentMode == 0) {  // Chỉ cho phép nhập mật khẩu mở khóa ở chế độ bình thường
      lcd.print("Nhap Mat Khau");
      lcd.setCursor(0, 1);
      lcd.print("Mo Khoa:");
      waitingForAuth = true;
      input = "";
      delay(1000);
      lcd.clear();
      lcd.print("Mat Khau:");
    } else {
      lcd.print("Moi xac thuc");
      waitingForAuth = true;
      input = "";
      delay(1000);
      lcd.clear();
      lcd.print("Nhap mat khau:");
    }
    return;
  }
  
  if (key == '*') {
    if (input.length() > 0) {
      input = input.substring(0, input.length() - 1);
      lcd.clear();
      lcd.print("Mat Khau:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < input.length(); i++) {
        lcd.print("*");
      }
    }
  }
  else if (key == '#') {
    if (!isAuthenticated && waitingForAuth) {
      if (currentMode == 0 && input == UNLOCK_PASSWORD) {  // Mật khẩu mở khóa cho chế độ bình thường
        successAccess();  // Sử dụng hàm successAccess() để mở khóa
        waitingForAuth = false;
        passwordAttempts = 0;
        wrongAttempts = 0;  // Reset số lần quét thẻ sai
        input = "";
      }
      else if (input == MASTER_PASSWORD) {  // Mật khẩu xác thực cho admin
        isAuthenticated = true;
        waitingForAuth = false;
        passwordAttempts = 0;
        wrongAttempts = 0;  // Reset số lần quét thẻ sai
        lcd.clear();
        lcd.print("Xac Thuc");
        lcd.setCursor(0, 1);
        lcd.print("Thanh Cong!");
        beepSuccess();
        delay(2000);
        displayWelcome();
      }
      else {
        passwordAttempts++;
        lcd.clear();
        lcd.print("Sai Mat Khau!");
        lcd.setCursor(0, 1);
        if (passwordAttempts < MAX_PASSWORD_ATTEMPTS) {
          beepError();
          delay(2000);
          lcd.clear();
          lcd.print("Nhap mat khau:");
        } else {
          lcd.print("Da Het Luot!");
          beepError();
          delay(2000);
          
          // Khóa hệ thống trong 10 giây
          lcd.clear();
          lcd.print("Khoa He Thong!");
          lcd.setCursor(0, 1);
          lcd.print("Doi: 10s");
          for (int i = 10; i > 0; i--) {
            lcd.setCursor(5, 1);
            lcd.print(i);
            lcd.print("s  ");
            beepError();
            delay(1000);
          }
          
          // Reset tất cả các biến về trạng thái ban đầu
          passwordAttempts = 0;
          waitingForAuth = false;
          wrongAttempts = 0;  // Reset số lần quét thẻ sai
          isAuthenticated = false;  // Reset trạng thái xác thực
          isSystemLocked = false;  // Reset system lock state
          currentMode = 0;  // Về chế độ quét thẻ
          displayWelcome();  // Hiển thị màn hình chào
          return;
        }
      }
      input = "";
    }
  }
  else if (waitingForAuth) {
    if (input.length() < 15) {
      input += key;
      lcd.clear();
      lcd.print("Mat Khau:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < input.length(); i++) {
        lcd.print("*");
      }
    }
  }
}

void updateModeDisplay() {
  lcd.clear();
  switch (currentMode) {
    case 1:
      lcd.print("Them The Chinh");
      lcd.setCursor(0, 1);
      lcd.print("Moi Quet The");
      break;
    case 2:
      lcd.print("Them The Phu");
      lcd.setCursor(0, 1);
      lcd.print("Moi Quet The");
      break;
    case 3:
      lcd.print("Xoa The");
      lcd.setCursor(0, 1);
      lcd.print("Moi Quet The");
      break;
    case 4:
      lcd.print("Xoa Toan Bo The");
      lcd.setCursor(0, 1);
      lcd.print("Quet The Chinh");
      break;
    default:
      displayWelcome();
      break;
  }
}

void beepSuccess() {
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

void beepError() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
}

void successAccess() {
  lcd.clear();
  lcd.print("Da Mo Khoa!");
  digitalWrite(LED_PIN, HIGH);
  beepSuccess();
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  wrongAttempts = 0;
  delay(1000);
  displayWelcome();  // Trở về màn hình chào
}

void failedAccess() {
  beepError();
  lcd.clear();
  lcd.print("Sai The!");
  lcd.setCursor(0, 1);
  if (wrongAttempts < 5) {
    
  } else {
    lcd.print("Nhap Mat Khau!");
  }
  delay(2000);
  
  if (wrongAttempts >= 5) {
    lcd.clear();
    lcd.print("Nhap Mat Khau:");
  } else {
    displayWelcome();
  }
}

bool checkCard(byte* cardUID) {
  // Kiểm tra cả thẻ chính và thẻ phụ
  return (checkMasterCard(cardUID) || checkGuestCard(cardUID));
}
bool checkMasterCard(byte* cardUID) {
  byte storedCard[4];
  for (int i = 0; i < 4; i++) {
    storedCard[i] = EEPROM.read(MASTER_CARD_ADDR + i);
  }
  return memcmp(cardUID, storedCard, 4) == 0;
}
bool checkGuestCard(byte* cardUID) {
  for (int i = 0; i < MAX_CARDS; i++) {
    int addr = GUEST_CARDS_ADDR + (i * CARD_SIZE);
    byte storedCard[4];
    bool isEmptySlot = true;
    // Đọc mã UID từ EEPROM
    for (int j = 0; j < 4; j++) {
      storedCard[j] = EEPROM.read(addr + j);
      // Kiểm tra xem vị trí này có trống không
      if (storedCard[j] != 255) {
        isEmptySlot = false;
      }
    }
    // Nếu vị trí không trống và mã UID khớp
    if (!isEmptySlot && memcmp(cardUID, storedCard, 4) == 0) {
      return true;
    }
  }
  return false;
}
void saveMasterCard(byte* cardUID) {
  // Lưu thẻ chính vào EEPROM
  for (int i = 0; i < 4; i++) {
    EEPROM.write(MASTER_CARD_ADDR + i, cardUID[i]);
  }
}

bool saveGuestCard(byte* cardUID) {
  // Tìm vị trí trống trong EEPROM để lưu thẻ mới
  for (int i = 0; i < MAX_CARDS; i++) {
    int addr = GUEST_CARDS_ADDR + (i * CARD_SIZE);
    bool isEmpty = true;
    
    // Kiểm tra xem vị trí này có trống không
    for (int j = 0; j < 4; j++) {
      if (EEPROM.read(addr + j) != 255) {
        isEmpty = false;
        break;
      }
    }  
    if (isEmpty) {
      // Lưu mã UID vào vị trí trống
      for (int j = 0; j < 4; j++) {
        EEPROM.write(addr + j, cardUID[j]);
      }
      return true;  // Thêm thẻ thành công
    }
  } 
  // Nếu không còn vị trí trống
  lcd.clear();
  lcd.print("Bo Nho Day!");
  beepError();
  delay(2000);
  displayWelcome();
  return false;  // Thêm thẻ thất bại
}
void deleteCard(byte* cardUID) {
  if (checkMasterCard(cardUID)) {
    lcd.clear();
    lcd.print("Khong The Xoa");
    lcd.setCursor(0, 1);
    lcd.print("The Chinh!");
    beepError();
    delay(2000);
    displayWelcome();
    return;
  }
  bool cardFound = false;
  // Tìm và xóa thẻ trong EEPROM
  for (int i = 0; i < MAX_CARDS; i++) {
    int addr = GUEST_CARDS_ADDR + (i * CARD_SIZE);
    byte storedCard[4];
    // Đọc mã UID từ EEPROM
    for (int j = 0; j < 4; j++) {
      storedCard[j] = EEPROM.read(addr + j);
    }
    // Nếu tìm thấy thẻ cần xóa
    if (memcmp(cardUID, storedCard, 4) == 0) {
      // Xóa mã UID bằng cách ghi giá trị 255
      for (int j = 0; j < 4; j++) {
        EEPROM.write(addr + j, 255);
      }
      cardFound = true;
      lcd.clear();
      lcd.print("Da Xoa The!");
      beepSuccess();
      delay(2000);
      displayWelcome();
      break;
    }
  }
  // Nếu không tìm thấy thẻ
  if (!cardFound) {
    lcd.clear();
    lcd.print("The Khong");
    lcd.setCursor(0, 1);
    lcd.print("Ton Tai!");
    beepError();
    delay(2000);
    displayWelcome();
  }
}
void deleteAllGuestCards() {
  // Xóa tất cả thẻ phụ bằng cách ghi giá trị 255
  for (int i = 0; i < MAX_CARDS; i++) {
    int addr = GUEST_CARDS_ADDR + (i * CARD_SIZE);
    for (int j = 0; j < 4; j++) {
      EEPROM.write(addr + j, 255);
    }
  }
  lcd.clear();
  lcd.print("Da Xoa Het The");
  beepSuccess();
  delay(2000);
  displayWelcome(); }

