// ============================================================
// 포켓 eBook 워크샵 코드
// Raspberry Pi Pico RP2040 + Waveshare 1.54" V2 e-paper
//
// 오늘은 아래의 "수정 영역"만 바꿔봅니다.
// 수정한 뒤 Arduino IDE의 업로드 버튼을 눌러 Pico에 넣어주세요.
// ============================================================

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <vector>
#include <string>


// ============================================================
// 여기만 수정해보세요
// ============================================================

// eBook 제목
// 현재 폰트에서는 한글이 제대로 표시되지 않을 수 있습니다.
// 화면 테스트는 영어/숫자로 먼저 해보는 것을 추천합니다.
const char* bookTitle = "My Pocket Ebook";

// eBook에 들어갈 문장들
// 문장 하나가 한 페이지처럼 사용됩니다.
// 문장을 추가하거나 바꾼 뒤 업로드해서 화면을 확인해보세요.
const char* bookPages[] = {
  "This is the first page.",
  "This is a slow little book.",
  "Press NEXT to turn the page.",
  "The screen remembers the text."
};

// 페이지 넘김 방식
// 0 = 마지막 페이지에서 멈추기
// 1 = 마지막 페이지 뒤에 처음 페이지로 돌아가기
const int pageMode = 1;


// ============================================================
// 아래 코드는 지금은 건드리지 않습니다
// ============================================================


// 전체 페이지 수를 자동으로 계산합니다.
// 문장을 추가해도 이 부분은 수정하지 않아도 됩니다.
const int bookPageCount = sizeof(bookPages) / sizeof(bookPages[0]);


// e-paper 연결 핀
#define EPD_MOSI  19
#define EPD_CLK   18
#define EPD_CS    17
#define EPD_DC    20
#define EPD_RST   21
#define EPD_BUSY  22

// 버튼 연결 핀
#define BTN_NEXT  15
#define BTN_PREV  14
#define BTN_HOME  5


// e-paper 디스플레이 설정
// BUSY 핀이 불안정할 경우 -1로 두고 delay 방식으로 동작합니다.
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, -1)
);


// 화면 글꼴과 여백 설정
const GFXfont* FONT = &FreeMonoBold9pt7b;

const int FONT_H    = 20;
const int MARGIN_X  = 8;
const int MARGIN_Y  = 28;
const int MAX_WIDTH = 184;
const int MAX_Y     = 188;


// 화면에 실제로 그릴 줄들을 저장하는 공간
std::vector<std::vector<std::string>> renderedPages;

// 현재 보고 있는 페이지 번호
int currentPage = 0;


// ------------------------------------------------------------
// 문자열의 화면 너비를 계산합니다.
// ------------------------------------------------------------
int measureWidth(const std::string& text) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);

  return (int)w;
}


// ------------------------------------------------------------
// 긴 문장을 e-paper 화면 너비에 맞게 여러 줄로 나눕니다.
// ------------------------------------------------------------
std::vector<std::string> wrapText(const char* text) {
  std::vector<std::string> lines;
  std::vector<std::string> words;

  std::string word;

  // 공백을 기준으로 단어를 나눕니다.
  for (const char* p = text; *p; p++) {
    if (*p == ' ' || *p == '\n' || *p == '\t') {
      if (!word.empty()) {
        words.push_back(word);
        word = "";
      }
    } else {
      word += *p;
    }
  }

  if (!word.empty()) {
    words.push_back(word);
  }

  std::string line;

  for (auto& w : words) {
    std::string testLine = line.empty() ? w : line + " " + w;

    if (measureWidth(testLine) > MAX_WIDTH) {
      if (!line.empty()) {
        lines.push_back(line);
        line = w;
      } else {
        lines.push_back(w);
        line = "";
      }
    } else {
      line = testLine;
    }
  }

  if (!line.empty()) {
    lines.push_back(line);
  }

  return lines;
}


// ------------------------------------------------------------
// 참가자가 작성한 bookPages를 화면에 그릴 수 있는 형태로 준비합니다.
// ------------------------------------------------------------
void buildBookPages() {
  renderedPages.clear();

  for (int i = 0; i < bookPageCount; i++) {
    std::vector<std::string> pageLines;

    // 제목을 첫 줄에 넣습니다.
    if (i == 0) {
      pageLines.push_back(std::string(bookTitle));
      pageLines.push_back("");
    }

    std::vector<std::string> wrapped = wrapText(bookPages[i]);

    for (auto& line : wrapped) {
      pageLines.push_back(line);
    }

    renderedPages.push_back(pageLines);
  }
}


// ------------------------------------------------------------
// 특정 페이지를 e-paper 화면에 표시합니다.
// ------------------------------------------------------------
void showPage(int index) {
  if (index < 0 || index >= (int)renderedPages.size()) {
    return;
  }

  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(FONT);
    display.setTextColor(GxEPD_BLACK);

    int y = MARGIN_Y;

    for (auto& line : renderedPages[index]) {
      // 빈 줄이면 한 줄 띄웁니다.
      if (line.length() == 0) {
        y += FONT_H;
        continue;
      }

      display.setCursor(MARGIN_X, y);
      display.print(line.c_str());
      y += FONT_H;

      if (y > MAX_Y) {
        break;
      }
    }

    // 오른쪽 아래에 현재 페이지 표시
    char indicator[16];
    snprintf(indicator, sizeof(indicator), "%d/%d", index + 1, bookPageCount);

    display.setCursor(150, 195);
    display.print(indicator);

  } while (display.nextPage());
}


// ------------------------------------------------------------
// 버튼이 눌렸는지 확인합니다.
// 버튼을 한 번 누를 때 한 번만 동작하도록 처리합니다.
// ------------------------------------------------------------
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);

    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW) {
        delay(10);
      }

      return true;
    }
  }

  return false;
}


// ------------------------------------------------------------
// 다음 페이지로 이동합니다.
// ------------------------------------------------------------
void goNextPage() {
  if (currentPage < bookPageCount - 1) {
    currentPage++;
  } else {
    // 마지막 페이지일 때의 동작
    if (pageMode == 1) {
      currentPage = 0;
    } else {
      currentPage = bookPageCount - 1;
    }
  }

  showPage(currentPage);
}


// ------------------------------------------------------------
// 이전 페이지로 이동합니다.
// ------------------------------------------------------------
void goPrevPage() {
  if (currentPage > 0) {
    currentPage--;
  }

  showPage(currentPage);
}


// ------------------------------------------------------------
// 첫 페이지로 이동합니다.
// ------------------------------------------------------------
void goHomePage() {
  currentPage = 0;
  showPage(currentPage);
}


// ------------------------------------------------------------
// 처음 한 번 실행되는 설정입니다.
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // 버튼 설정
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_HOME, INPUT_PULLUP);

  // SPI 설정
  SPI.setSCK(EPD_CLK);
  SPI.setTX(EPD_MOSI);
  SPI.begin();

  // e-paper 설정
  display.init(115200);
  display.setRotation(2);
  display.setFont(FONT);
  display.setTextColor(GxEPD_BLACK);

  // 페이지 준비 후 첫 화면 표시
  buildBookPages();
  showPage(currentPage);
}


// ------------------------------------------------------------
// 계속 반복 실행되는 부분입니다.
// 버튼 입력을 확인하고 페이지를 바꿉니다.
// ------------------------------------------------------------
void loop() {
  if (buttonPressed(BTN_NEXT)) {
    goNextPage();
  }

  if (buttonPressed(BTN_PREV)) {
    goPrevPage();
  }

  if (buttonPressed(BTN_HOME)) {
    goHomePage();
  }
}