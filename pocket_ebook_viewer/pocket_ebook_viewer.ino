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

// 이미지 하나의 정보
// 이미지 데이터 이름, 가로 크기, 세로 크기를 함께 저장합니다.
struct BitmapInfo {
  const unsigned char* data;
  int width;
  int height;
};


// ============================================================
// 여기만 수정해보세요
// ============================================================

// eBook 제목
// 현재 폰트에서는 한글이 제대로 표시되지 않을 수 있습니다.
// 화면 테스트는 영어/숫자로 먼저 해보는 것을 추천합니다.
const char* bookTitle = "";

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

// 이미지 데이터
// 이미지를 넣고 싶으면 아래 변환기에서 만든 비트맵 데이터를 { } 안에 붙여넣습니다.
//
// 이미지 to 비트맵 변환기:
// https://javl.github.io/image2cpp/
//
// 추천 설정:
// - 크기: 64 x 64
// - 출력 형식: C/C++ 배열
// - 색상: 흑백
//
// 변환한 비트맵은 myBitmap의 = 오른쪽 { } 안에 붙여넣습니다.
//
// 예:
// const std::vector<unsigned char> myBitmap = {
//   0x00, 0x18, 0x3C, 0x7E, ...
// };
const std::vector<unsigned char> myBitmap = {
  0x00 // 여기에 이미지 데이터 붙여넣기
};

// 두 번째 이미지가 필요하면 여기에 붙여넣습니다.
// 필요 없으면 비워둡니다.
const std::vector<unsigned char> myBitmap2 = {
  0x00 // 여기에 두 번째 이미지 데이터 붙여넣기
};

// 이미지 크기
// 변환한 이미지의 크기에 맞게 수정합니다.
// 1.54인치 e-paper에서는 64x64 또는 80x80 정도를 추천합니다.
// 전체 화면 200x200 이미지를 쓰고 싶으면 width와 height를 0으로 둡니다.
const int myBitmapWidth = 0;
const int myBitmapHeight = 0;

const int myBitmap2Width = 0;
const int myBitmap2Height = 0;

// eBook에 들어갈 이미지들
// 이미지를 추가하려면 위에 이미지 데이터를 만들고, 아래 목록에 추가합니다.
// 이 목록도 직접 수정하는 곳입니다.
//
// 예:
// { myBitmap.data(), 64, 64 }
//
// 전체 화면 200x200 이미지는 이렇게 적을 수 있습니다.
// { myFullScreenBitmap.data(), 0, 0 }
const BitmapInfo bookBitmaps[] = {
  { myBitmap.data(), myBitmapWidth, myBitmapHeight },
  { myBitmap2.data(), myBitmap2Width, myBitmap2Height }
};

// 페이지 순서 정하기
// TEXT_PAGE(0)은 bookPages[0]의 문장을 보여줍니다.
// IMAGE_PAGE(0)은 bookBitmaps[0]의 이미지를 보여줍니다.
// 이 목록도 직접 수정하는 곳입니다.
//
// 텍스트만 쓰고 싶으면 IMAGE_PAGE 줄을 지워도 됩니다.
// 이미지가 필요 없으면 IMAGE_PAGE 줄을 지워도 됩니다.
enum PageType {
  PAGE_TEXT,
  PAGE_BITMAP
};

struct PageOrder {
  PageType type;
  int index;
};

#define TEXT_PAGE(pageIndex) \
  { PAGE_TEXT, pageIndex }

#define IMAGE_PAGE(bitmapIndex) \
  { PAGE_BITMAP, bitmapIndex }

const PageOrder pages[] = {
  TEXT_PAGE(0),
  TEXT_PAGE(1),
  IMAGE_PAGE(0),
  TEXT_PAGE(2),
  IMAGE_PAGE(1),
  TEXT_PAGE(3)
};


// ============================================================
// 아래 코드는 지금은 건드리지 않습니다
// ============================================================


// 전체 페이지 수를 자동으로 계산합니다.
const int pageCount = sizeof(pages) / sizeof(pages[0]);
const int bookPageCount = sizeof(bookPages) / sizeof(bookPages[0]);
const int bitmapPageCount = sizeof(bookBitmaps) / sizeof(bookBitmaps[0]);


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


// 화면에 실제로 그릴 텍스트 줄들을 저장하는 공간
std::vector<std::vector<std::string>> renderedTextPages;

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
void buildTextPages() {
  renderedTextPages.clear();

  for (int i = 0; i < bookPageCount; i++) {
    std::vector<std::string> pageLines;

    if (i == 0) {
      pageLines.push_back(std::string(bookTitle));
      pageLines.push_back("");
    }

    std::vector<std::string> wrapped = wrapText(bookPages[i]);
    for (auto& line : wrapped) {
      pageLines.push_back(line);
    }

    renderedTextPages.push_back(pageLines);
  }
}


// ------------------------------------------------------------
// 페이지 번호를 오른쪽 아래에 표시합니다.
// ------------------------------------------------------------
void drawPageIndicator(int index) {
  char indicator[16];
  snprintf(indicator, sizeof(indicator), "%d/%d", index + 1, pageCount);

  display.setCursor(150, 195);
  display.print(indicator);
}


// ------------------------------------------------------------
// 텍스트 페이지를 그립니다.
// ------------------------------------------------------------
void drawTextPage(int index) {
  if (index < 0 || index >= bookPageCount) {
    return;
  }

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
// bitmap, width, height를 파라미터로 받아서 그립니다.
// ------------------------------------------------------------
void drawBitmapPage(const unsigned char* bitmap, int bitmapWidth, int bitmapHeight) {
  if (bitmapWidth <= 0 || bitmapHeight <= 0) {
    bitmapWidth = display.width();
    bitmapHeight = display.height();
  }

  int x = (display.width() - bitmapWidth) / 2;
  int y = (display.height() - bitmapHeight) / 2;

  display.drawBitmap(
    x,
    y,
    bitmap,
    bitmapWidth,
    bitmapHeight,
    GxEPD_BLACK
  );
}


// ------------------------------------------------------------
// 이미지 번호에 맞는 그림을 그립니다.
// ------------------------------------------------------------
void drawBitmapByIndex(int bitmapIndex) {
  if (bitmapIndex >= 0 && bitmapIndex < bitmapPageCount) {
    BitmapInfo bitmap = bookBitmaps[bitmapIndex];
    drawBitmapPage(bitmap.data, bitmap.width, bitmap.height);
  }
}


// ------------------------------------------------------------
// 특정 페이지를 e-paper 화면에 표시합니다.
// pages[]의 type에 따라 텍스트 또는 이미지를 그립니다.
// ------------------------------------------------------------
void showPage(int index) {
  if (index < 0 || index >= pageCount) {
    return;
  }

  PageOrder page = pages[index];

  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(FONT);
    display.setTextColor(GxEPD_BLACK);

    if (page.type == PAGE_TEXT) {
      drawTextPage(page.index);
    } else if (page.type == PAGE_BITMAP) {
      drawBitmapByIndex(page.index);
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
// ------------------------------------------------------------
void goNextPage() {
  if (currentPage < pageCount - 1) {
    currentPage++;
  } else if (pageMode == 1) {
    currentPage = 0;
  } else {
    currentPage = pageCount - 1;
  }

  showPage(currentPage);
}


// ------------------------------------------------------------
// 이전 페이지로 이동합니다.
// ------------------------------------------------------------
void goPrevPage() {
  if (currentPage > 0) {
    currentPage--;
  } else if (pageMode == 1) {
    currentPage = pageCount - 1;
  } else {
    currentPage = 0;
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

  if (buttonPressed(BTN_HOME)) {
    goHomePage();
  }
}
