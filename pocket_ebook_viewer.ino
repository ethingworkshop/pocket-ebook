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

// eBook에 들어갈 문장들
// 문장 하나가 한 페이지처럼 사용됩니다.
// 문장을 추가하거나 바꾼 뒤 업로드해서 화면을 확인해보세요.
const char* bookPages[] = {
  "This is the first page.",
  "This is a slow little book.",
  "Press NEXT to turn the page.",
  "The screen remembers the text."
};


// 이미지 데이터
// 이미지를 넣고 싶으면 변환한 비트맵 데이터를 아래 { } 안에 붙여넣습니다.
// 이미지를 쓰지 않으려면 비워둡니다.
//
// 예:
// const std::vector<unsigned char> myBitmap = {
//   0x00, 0x18, 0x3C, 0x7E, ...
// };
const std::vector<unsigned char> myBitmap = {
  // 여기에 이미지 데이터 붙여넣기
};

// 이미지 크기
// 변환한 이미지의 크기에 맞게 수정합니다.
// 1.54인치 e-paper에서는 64x64 또는 80x80 정도를 추천합니다.
const int myBitmapWidth = 64;
const int myBitmapHeight = 64;


// ============================================================
// 아래 코드는 지금은 건드리지 않습니다
// ============================================================


// 전체 텍스트 페이지 수를 자동으로 계산합니다.
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


// 화면에 실제로 그릴 텍스트 줄들을 저장하는 공간
std::vector<std::vector<std::string>> renderedTextPages;

// 현재 보고 있는 페이지 번호
int currentPage = 0;


// ------------------------------------------------------------
// 이미지가 들어 있는지 확인합니다.
// myBitmap이 비어 있으면 이미지 페이지를 만들지 않습니다.
// ------------------------------------------------------------
bool hasBitmapPage() {
  return myBitmap.size() > 0;
}


// ------------------------------------------------------------
// 전체 페이지 수를 계산합니다.
// 텍스트 페이지 + 이미지 페이지가 있으면 1페이지 추가
// ------------------------------------------------------------
int totalPageCount() {
  if (hasBitmapPage()) {
    return bookPageCount + 1;
  }

  return bookPageCount;
}


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
void buildTextPages() {
  renderedTextPages.clear();

  for (int i = 0; i < bookPageCount; i++) {
    std::vector<std::string> pageLines = wrapText(bookPages[i]);
    renderedTextPages.push_back(pageLines);
  }
}


// ------------------------------------------------------------
// 페이지 번호를 오른쪽 아래에 표시합니다.
// ------------------------------------------------------------
void drawPageIndicator(int index) {
  char indicator[16];
  snprintf(indicator, sizeof(indicator), "%d/%d", index + 1, totalPageCount());

  display.setCursor(150, 195);
  display.print(indicator);
}


// ------------------------------------------------------------
// 텍스트 페이지를 그립니다.
// ------------------------------------------------------------
void drawTextPage(int index) {
  int y = MARGIN_Y;

  for (auto& line : renderedTextPages[index]) {
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
}


// ------------------------------------------------------------
// 이미지 페이지를 그립니다.
// 텍스트 페이지를 모두 지난 다음 마지막에 표시됩니다.
// ------------------------------------------------------------
void drawBitmapPage() {
  if (!hasBitmapPage()) {
    return;
  }

  int x = (display.width() - myBitmapWidth) / 2;
  int y = (display.height() - myBitmapHeight) / 2;

  display.drawBitmap(
    x,
    y,
    myBitmap.data(),
    myBitmapWidth,
    myBitmapHeight,
    GxEPD_BLACK
  );
}


// ------------------------------------------------------------
// 특정 페이지를 e-paper 화면에 표시합니다.
// 텍스트 페이지가 먼저 나오고,
// 이미지 데이터가 있으면 마지막에 이미지 페이지가 나옵니다.
// ------------------------------------------------------------
void showPage(int index) {
  if (index < 0 || index >= totalPageCount()) {
    return;
  }

  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(FONT);
    display.setTextColor(GxEPD_BLACK);

    if (index < bookPageCount) {
      drawTextPage(index);
    } else {
      drawBitmapPage();
    }

    drawPageIndicator(index);

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
// 마지막 페이지에서 NEXT를 누르면 첫 페이지로 돌아갑니다.
// ------------------------------------------------------------
void goNextPage() {
  if (currentPage < totalPageCount() - 1) {
    currentPage++;
  } else {
    currentPage = 0;
  }

  showPage(currentPage);
}


// ------------------------------------------------------------
// 이전 페이지로 이동합니다.
// 첫 페이지에서 PREV를 누르면 마지막 페이지로 이동합니다.
// ------------------------------------------------------------
void goPrevPage() {
  if (currentPage > 0) {
    currentPage--;
  } else {
    currentPage = totalPageCount() - 1;
  }

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

  // SPI 설정
  SPI.setSCK(EPD_CLK);
  SPI.setTX(EPD_MOSI);
  SPI.begin();

  // e-paper 설정
  display.init(115200);
  display.setRotation(2);
  display.setFont(FONT);
  display.setTextColor(GxEPD_BLACK);

  // 텍스트 페이지를 준비하고 첫 화면을 표시합니다.
  buildTextPages();
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
}