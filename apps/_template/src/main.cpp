#include <M5Cardputer.h>

namespace {

constexpr const char* kAppName = "APP TEMPLATE";
constexpr uint32_t kTextColor = GREEN;
constexpr uint32_t kBgColor = BLACK;

void drawHomeScreen(bool anyKeyDown, char lastKey) {
  M5Cardputer.Display.fillScreen(kBgColor);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(12, 20);
  M5Cardputer.Display.setTextColor(kTextColor, kBgColor);
  M5Cardputer.Display.println(kAppName);

  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(12, 62);
  M5Cardputer.Display.println("Cardputer-Adv starter");
  M5Cardputer.Display.setCursor(12, 80);
  M5Cardputer.Display.println("Press any key to test input");
  M5Cardputer.Display.setCursor(12, 108);

  if (anyKeyDown) {
    M5Cardputer.Display.printf("Last key: %c", lastKey);
  } else {
    M5Cardputer.Display.print("Last key: none");
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  Serial.begin(115200);
  delay(120);
  Serial.println("Cardputer-Adv app template boot");

  drawHomeScreen(false, '\0');
}

void loop() {
  M5Cardputer.update();

  static bool lastHadKey = false;
  static char lastKey = '\0';

  bool hasKey = false;
  char pressedKey = lastKey;

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
      if (!status.word.empty()) {
        pressedKey = status.word[0];
        hasKey = true;
        Serial.printf("Key pressed: %c\n", pressedKey);
      }
    }
  }

  if (hasKey || lastHadKey != hasKey) {
    lastHadKey = hasKey;
    if (hasKey) {
      lastKey = pressedKey;
    }
    drawHomeScreen(hasKey, lastKey);
  }

  delay(20);
}
