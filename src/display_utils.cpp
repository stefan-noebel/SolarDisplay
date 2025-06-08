#include "display_utils.h"
#include "FreeSansBold32pt7b.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Arduino.h>
// Include ePaper Display library
#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include <GxEPD2_3C.h> // including both doesn't use more code or ram
#include "GxEPD2_display_selection_new_style.h"

// Extern variables from main.cpp
extern const char greeting[];
extern int16_t tbx, tby;
extern uint16_t tbw, tbh;
extern double_t ACpower, ACtotal, ACdaily;

void drawGreeting()
{
    display.setFont(&FreeSansBold32pt7b);
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
    display.getTextBounds(greeting, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x1 = ((display.width() - tbw) / 2) - tbx;
    uint16_t y1 = ((display.height() - tbh) / 4) - tby;
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x1, y1);
        display.print(greeting);
    } while (display.nextPage());
    display.hibernate();
}

void drawRefresh()
{
    tbh = nearbyint(ACpower / 600 * 94);
    tbw = nearbyint(ACdaily / 5 * 198);

    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
    display.setPartialWindow(0, 0, display.width(), display.height());
    display.firstPage();
    do {
        display.setFont(&FreeSansBold32pt7b);
        display.setCursor(0, 79);
        display.printf("%3.0f", ACpower);
        display.setFont(&FreeSansBold18pt7b);
        display.setCursor(68, 128);
        display.printf("%1.1f", ACdaily);
        display.setCursor(68, 199);
        display.printf("%4.0f", ACtotal);

        display.fillRect(165, 95 - tbh, 32, tbh, GxEPD_BLACK);
        display.fillRect(1, 133, tbw, 14, GxEPD_BLACK);

        display.setFont(&FreeSans18pt7b);
        display.setCursor(106, 79);
        display.print("W");
        display.setFont(&FreeSans12pt7b);
        display.setCursor(0, 17);
        display.print("Leistung");
        display.setCursor(124, 17);
        display.print("600");
        display.setCursor(150, 95);
        display.print("0");
        display.drawRect(164, 0, 35, 96, GxEPD_BLACK);
        display.drawRect(0, 99, 200, 2, GxEPD_BLACK);
        display.drawRect(0, 132, 200, 16, GxEPD_BLACK);
        display.drawRect(0, 171, 200, 2, GxEPD_BLACK);
        display.setCursor(0, 128);
        display.print("heute");
        display.setCursor(117, 128);
        display.print("kWh");
        display.setCursor(0, 167);
        display.print("0");
        display.setCursor(188, 167);
        display.print("5");
        display.setCursor(0, 199);
        display.print("total");
        display.setCursor(146, 199);
        display.print("kWh");
    } while (display.nextPage());
    display.powerOff();
    display.hibernate();
}