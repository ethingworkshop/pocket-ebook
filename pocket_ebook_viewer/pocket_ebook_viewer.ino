// Pocket E-Book
// Raspberry Pi Pico RP2040 + Waveshare 1.54" V2 e-paper

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <vector>
#include <string>

// 이미지 메타 데이터
struct BitmapInfo {
  const unsigned char* data;
  int width;
  int height;
};

// 페이지의 종류
// PAGE_TEXT   = 글 페이지
// PAGE_BITMAP = 이미지 페이지
enum PageType {
  PAGE_TEXT,
  PAGE_BITMAP
};

// 실제 책의 페이지 순서 정보
struct PageOrder {
  PageType type;
  int index;
};

// macro for { struct, variable}
#define TEXT_PAGE(pageIndex) \
  { PAGE_TEXT, pageIndex }

#define IMAGE_PAGE(bitmapIndex) \
  { PAGE_BITMAP, bitmapIndex }

// 1. 문장 바꾸기

// eBook에 들어갈 문장들
// 문장 하나가 한 페이지처럼 사용됩니다.
//
// bookPages[0] = 첫 번째 문장
// bookPages[1] = 두 번째 문장
// bookPages[2] = 세 번째 문장
const char* bookPages[] = {
  "This is the first page.",
  "This is a slow little book.",
  "Press NEXT to turn the page.",
  "The screen remembers the text."
};


// 2. 이미지 붙여넣기

// 이미지는 image2cpp에서 200 * 200 흑백 이미지로 변환한 뒤,
// 아래 { } 안에 붙여넣습니다.
//
// 이미지 to 비트맵 변환기:
// https://javl.github.io/image2cpp/
//
// 추천 설정:
// - 크기: 200 * 200
// - 출력 형식: C/C++ 배열
// - 색상: 흑백
//
// 예:
// const std::vector<unsigned char> myBitmap = {
//   0x00, 0x18, 0x3C, 0x7E, ...
// };
//
// 한글이나 이모지는 글자로 출력하기보다,
// 이미지로 만들어서 넣는 것을 추천합니다.

const std::vector<unsigned char> myBitmap = {
  0x00 // 첫 번째 이미지 데이터 붙여넣기
};

const std::vector<unsigned char> myBitmap2 = {
  0x00 // 두 번째 이미지 데이터 붙여넣기
};

// 이미지를 더 넣고 싶다면 위 형식을 복사해서 추가할 수 있습니다.
//
// 예:
// const std::vector<unsigned char> myBitmap3 = {
//   0x00 // 세 번째 이미지 데이터 붙여넣기
// };


// 3. 페이지 순서 바꾸기

// 여기에서 실제 eBook의 페이지 순서를 정합니다. 나열 순서가 곧 페이지 순서.
//
// TEXT_PAGE(0)  = bookPages[0]의 문장
// TEXT_PAGE(1)  = bookPages[1]의 문장
// IMAGE_PAGE(0) = myBitmap 이미지
// IMAGE_PAGE(1) = myBitmap2 이미지
//
// 이미지는 여러 개 넣을 수 있지만,
// 한 페이지에는 이미지 하나만 보여줍니다.
const PageOrder pages[] = {
  TEXT_PAGE(0),
  TEXT_PAGE(1),
  IMAGE_PAGE(0),
  TEXT_PAGE(2),
  IMAGE_PAGE(1),
  TEXT_PAGE(3)
};


// 이미지 목록 만들기

// 위에서 붙여넣은 이미지들을 책에서 사용할 수 있도록 목록으로 만듭니다.
//
// myBitmap  -> IMAGE_PAGE(0)
// myBitmap2 -> IMAGE_PAGE(1)
const BitmapInfo bookBitmaps[] = {
  { myBitmap.data(), 200, 200 },
  { myBitmap2.data(), 200, 200 }

  // 세 번째 이미지를 추가했다면 아래처럼 한 줄을 더 넣을 수 있습니다.
  // , { myBitmap3.data(), width, height }
};


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


// 문자열의 화면 너비를 계산합니다.
int measureWidth(const std::string& text) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);

  return (int)w;
}


// 긴 문장을 e-paper 화면 너비에 맞게 여러 줄로 나눕니다.
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


// 참가자가 작성한 bookPages를 화면에 그릴 수 있는 형태로 준비합니다.
void buildTextPages() {
  renderedTextPages.clear();

  for (int i = 0; i < bookPageCount; i++) {
    std::vector<std::string> pageLines;

    std::vector<std::string> wrapped = wrapText(bookPages[i]);
    for (auto& line : wrapped) {
      pageLines.push_back(line);
    }

    renderedTextPages.push_back(pageLines);
  }
}


// 페이지 번호를 오른쪽 아래에 표시합니다.
void drawPageIndicator(int index) {
  char indicator[16];
  snprintf(indicator, sizeof(indicator), "%d/%d", index + 1, pageCount);

  display.setCursor(150, 195);
  display.print(indicator);
}


// 텍스트 페이지를 그립니다.
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


// 이미지 페이지를 그립니다.
// 이미지 데이터와 크기를 받아서 화면 가운데에 그립니다.
void drawBitmapPage(const unsigned char* bitmap, int bitmapWidth, int bitmapHeight) {
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


// 이미지 번호에 맞는 그림을 그립니다.
// IMAGE_PAGE(0), IMAGE_PAGE(1) 같은 번호가 여기로 들어옵니다.
void drawBitmapByIndex(int bitmapIndex) {
  if (bitmapIndex >= 0 && bitmapIndex < bitmapPageCount) {
    BitmapInfo bitmap = bookBitmaps[bitmapIndex];
    drawBitmapPage(bitmap.data, bitmap.width, bitmap.height);
  }
}


// 특정 페이지를 e-paper 화면에 표시합니다.
// pages[]의 type에 따라 텍스트 또는 이미지를 그립니다.
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


// 버튼이 눌렸는지 확인합니다.
// 버튼을 한 번 누를 때 한 번만 동작하도록 처리합니다.
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(20);

    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW) {
        delay(5);
      }

      return true;
    }
  }

  return false;
}


// 다음 페이지로 이동합니다.
void goNextPage() {
  if (currentPage < pageCount - 1) {
    currentPage++;
  } else {
    currentPage = 0;
  }
  showPage(currentPage);
}


// 이전 페이지로 이동합니다.
void goPrevPage() {
  if (currentPage > 0) {
    currentPage--;
  } else {
    currentPage = pageCount - 1;
  }

  showPage(currentPage);
}


// 첫 페이지로 이동합니다.
void goHomePage() {
  currentPage = 0;
  showPage(currentPage);
}


// 처음 한 번 실행되는 설정입니다.
void setup() {
  Serial.begin(115200);
  delay(500);

  // 버튼 설정
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
;

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


// 계속 반복 실행되는 부분입니다.
// 버튼 입력을 확인하고 페이지를 바꿉니다.
void loop() {
  if (buttonPressed(BTN_NEXT)) {
    goNextPage();
  }

  if (buttonPressed(BTN_PREV)) {
    goPrevPage();
  }

}
