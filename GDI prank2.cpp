#include <windows.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <mmsystem.h>             // Pro funkci PlaySound
#include "resource.h"
#pragma comment(lib, "winmm.lib") // Automaticky připojí multimediální knihovnu Windows


// Aktivuje plné rozlišení pro monitor, aby efekty nebyly rozmazané
void MakeDpiAware() {
    SetProcessDPIAware();
}

// Zpracování zpráv neviditelného okna (zajišťuje blokování Alt+F4)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        return 0; // Blokuje pokus o zavření

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE) {
            return 0; // Blokuje systémový příkaz pro zavření (Alt + F4)
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// wWinMain je správný vstupní bod pro "Desktopovou aplikaci pro Windows"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    MakeDpiAware();

    // 1. Vytvoření neviditelného okna na pozadí, které chytá zkratky
    const wchar_t CLASS_NAME[] = L"GdiBlockerClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"PrankApp", 0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );

    // 2. Příprava grafického kontextu a rozlišení
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    HICON iconError = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    const float cycleDuration = 75.0f;
    const float holdRequiredSeconds = 5.0f;

    MSG msg = { };

    // ==========================================
    // 1. BEZRÁMEČKOVÉ OKNO S ODPOČTEM (10 sekund)
    // ==========================================
    int winW = 450, winH = 160;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASS wcCount = { 0 };
    wcCount.lpfnWndProc = DefWindowProc;
    wcCount.hInstance = GetModuleHandle(NULL);
    wcCount.lpszClassName = L"CountdownWindowClass";
    wcCount.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wcCount);

    HWND hwndCount = CreateWindowEx(
        WS_EX_TOPMOST, L"CountdownWindowClass", NULL,
        WS_POPUP | WS_VISIBLE,
        (screenW - winW) / 2, (screenH - winH) / 2, winW, winH,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    // Vynutíme zobrazení a popředí okna
    ShowWindow(hwndCount, SW_SHOW);
    SetForegroundWindow(hwndCount);

    HFONT hFontText = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
    HFONT hFontNum = CreateFont(48, 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
    HFONT hFontSign = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");

    for (int sec = 10; sec >= 0; sec--) {
        // 1. Vyžádáme si čerstvý HDC přímo pro toto překreslení
        HDC hdcWin = GetDC(hwndCount);
        SetBkMode(hdcWin, TRANSPARENT);

        // Vyčištění plochy okna
        RECT rectWin = { 0, 0, winW, winH };
        FillRect(hdcWin, &rectWin, (HBRUSH)GetStockObject(BLACK_BRUSH));

        // Text: ATOMIC BOMB IN:
        SelectObject(hdcWin, hFontText);
        SetTextColor(hdcWin, RGB(255, 50, 50));
        RECT rText = { 10, 20, winW - 10, 50 };
        DrawText(hdcWin, L"ATOMIC BOMB IN:", -1, &rText, DT_CENTER | DT_SINGLELINE);

        // Číslo odpočtu
        SelectObject(hdcWin, hFontNum);
        if (sec <= 3) {
            SetTextColor(hdcWin, RGB(255, 30, 30));  // Zářivě červená
        }
        else {
            SetTextColor(hdcWin, RGB(255, 255, 255)); // Čistě bílá
        }

        wchar_t numStr[20];
        swprintf_s(numStr, L"%d...", sec);

        RECT rNum = { 0, 65, winW, 140 };
        DrawText(hdcWin, numStr, -1, &rNum, DT_CENTER | DT_SINGLELINE);

        // Podpis autora vlevo dole (šedě)
        SelectObject(hdcWin, hFontSign);
        SetTextColor(hdcWin, RGB(120, 120, 120));
        RECT rSign = { 12, winH - 22, winW - 10, winH - 5 };
        DrawText(hdcWin, L"by Filasss666", -1, &rSign, DT_LEFT | DT_SINGLELINE);

        // Uvolníme DC
        ReleaseDC(hwndCount, hdcWin);

        // 2. Vynutíme okno k okamžitému překreslení na obrazovku!
        RedrawWindow(hwndCount, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

        // 3. Prohneme zprávy okna
        MSG m;
        while (PeekMessage(&m, hwndCount, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m);
            DispatchMessage(&m);
        }
        

        // 3. Pípaní při každé sekunde
        if (sec > 3) {
            Beep(800, 100);
            Sleep(900); // 100 ms beep + 900 ms wait = 1 s
        }
        else if (sec > 0) {
            Beep(1500, 150); // Vyšší tón pro 3, 2, 1
            Sleep(850);
        }
        else {
            Beep(2000, 400); // Finální dlouhý tón
            Sleep(600);
        }
    }

    DeleteObject(hFontText);
    DeleteObject(hFontNum);
    DestroyWindow(hwndCount);
    DeleteObject(hFontSign);

    // ==========================================
    // 2. START HUDY A ČASU PRO GDI EFEKTY
    // ==========================================
    auto startTime = std::chrono::steady_clock::now();
    auto escPressStart = std::chrono::steady_clock::now();
    bool isEscHolding = false;

    PlaySound(MAKEINTRESOURCE(131), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);

    // 3. Hlavní smyčka aplikace
    while (true) {
        // Zpracování zpráv na pozadí (udržuje blokování Alt+F4)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // --- PLYNULÉ TŘESENÍ MYŠÍ (HNED OD ZAČÁTKU) ---
        static DWORD lastMouseJitter = 0;
        DWORD now = GetTickCount();

        // První posun proběhne okamžitě, další pak každých 15 ms
        if (lastMouseJitter == 0 || (now - lastMouseJitter >= 15)) {
            POINT p;
            if (GetCursorPos(&p)) {
                // Posun v rozsahu -5 až +5 pixelů
                int offsetX = (rand() % 11) - 5;
                int offsetY = (rand() % 11) - 5;

                SetCursorPos(p.x + offsetX, p.y + offsetY);
            }
            lastMouseJitter = now;
        }

        auto currentTime = std::chrono::steady_clock::now();
        float totalElapsed = std::chrono::duration<float>(currentTime - startTime).count();
        float elapsed = fmod(totalElapsed, cycleDuration);

        // --- KONTROLA UKONČENÍ: ESC PODRŽENÉ NA 5 SEKUND ---
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            if (!isEscHolding) {
                isEscHolding = true;
                escPressStart = std::chrono::steady_clock::now();
            }
            float heldDuration = std::chrono::duration<float>(currentTime - escPressStart).count();

            if (heldDuration >= holdRequiredSeconds) {
                break; // Úspěšné vypnutí
            }
        }
        else {
            isEscHolding = false;
        }

        // --- FÁZE 1 (0–5 s): DROGOVÝ TRIP (Invert barev + Skákající ikony) ---
        if (elapsed < 5.0f) {
            // 1. Invertování barev na celém displeji (šilené barvy)
            if ((rand() % 10) > 2) {
                PatBlt(hdc, 0, 0, sw, sh, PATINVERT);
            }

            // 2. Skákající ikony chyb/varování po obrazovce (jako ve videu)
            for (int i = 0; i < 5; i++) {
                int iconX = rand() % sw;
                int iconY = rand() % sh;

                // Vykreslení ikon na náhodné souřadnice
                DrawIcon(hdc, iconX, iconY, LoadIcon(NULL, IDI_ERROR));
                DrawIcon(hdc, iconX + 20, iconY + 20, LoadIcon(NULL, IDI_WARNING));
            }

            // 3. Lehké horizontální rozmazání/vlnění pod barvami
            int shiftX = (int)(sin(GetTickCount() / 100.0f) * 15.0f);
            BitBlt(hdc, shiftX, 0, sw, sh, hdc, 0, 0, SRCCOPY);
        }

        // --- FÁZE 2 (5–10 s): ANALOG TV GLITCH & SLOUPEC IKON ---
        else if (elapsed < 10.0f) {
            // 1. Trhání obrazovky na vodorovné pruhy (Scanline Displacement)
            for (int i = 0; i < 3; i++) {
                int h = rand() % 40 + 5;           // Výška pruhu
                int y = rand() % (sh - h);         // Náhodná výška na obrazovce
                int shiftX = (rand() % 60) - 30;   // Posun vlevo nebo vpravo
                BitBlt(hdc, shiftX, y, sw, h, hdc, 0, y, SRCCOPY);
            }

            // 2. Analogový šum a zrnění (Invertování náhodných pásů)
            if (rand() % 2 == 0) {
                int noiseY = rand() % sh;
                int noiseH = rand() % 60 + 10;
                PatBlt(hdc, 0, noiseY, sw, noiseH, PATINVERT);
            }

            // 3. Rozpůlení a inverze obrazovky (Split Screen Glitch)
            if (rand() % 4 == 0) {
                int splitY = sh / 2;
                BitBlt(hdc, 0, 0, sw, splitY, hdc, 0, splitY, SRCINVERT);
            }

            // 4. Sloupec / kaskáda ikon padajících uprostřed obrazovky
            static int iconY = 0;
            iconY = (iconY + 30) % sh;
            int centerX = (sw / 2) + ((rand() % 40) - 20);

            DrawIcon(hdc, centerX, iconY, LoadIcon(NULL, IDI_ERROR));
            DrawIcon(hdc, centerX, (iconY + 50) % sh, LoadIcon(NULL, IDI_WARNING));
            DrawIcon(hdc, centerX + ((rand() % 30) - 15), rand() % sh, LoadIcon(NULL, IDI_INFORMATION));
        }

        // --- FÁZE 3 (10–15 s): Screen Melting ---
        else if (elapsed < 15.0f) {
            for (int i = 0; i < 20; i++) {
                int dropX = rand() % sw;
                int dropW = (rand() % 70) + 10;
                int dropSpeed = (rand() % 20) + 5;
                BitBlt(hdc, dropX, dropSpeed, dropW, sh, hdc, dropX, 0, SRCCOPY);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        // --- FÁZE 4 (15–20 s): Tunel ---
        else if (elapsed < 20.0f) {
            int pad = 12;
            StretchBlt(hdc, pad, pad, sw - (2 * pad), sh - (2 * pad), hdc, 0, 0, sw, sh, SRCCOPY);
        }
        // --- FÁZE 5 (20–25 s): Diagonální mřížka ---
        else if (elapsed < 25.0f) {
            for (int i = 0; i < sh; i += 8) {
                int shift = static_cast<int>(sin(i / 15.0f + totalElapsed * 10.0f) * 35.0f);
                BitBlt(hdc, shift, i, sw, 8, hdc, 0, i, SRCCOPY);
            }
        }
        // --- FÁZE 6 (25–30 s): Apokalypsa s chybovými ikonami ---
        else if (elapsed < 30.0f) {
            BitBlt(hdc, (rand() % 80) - 40, (rand() % 80) - 40, sw, sh, hdc, 0, 0, SRCCOPY);
            for (int i = 0; i < 6; i++) {
                DrawIcon(hdc, rand() % sw, rand() % sh, iconError);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        // --- FÁZE 7 (30–40 s): HYDROGEN TRUE PIXEL SHADER ---
        else if (elapsed < 40.0f) {
            HDC memDC = CreateCompatibleDC(hdc);
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = sw;
            bmi.bmiHeader.biHeight = -sh;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            RGBQUAD* pixels = nullptr;
            HBITMAP hDIB = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
            SelectObject(memDC, hDIB);

            BitBlt(memDC, 0, 0, sw, sh, hdc, 0, 0, SRCCOPY);

            for (int y = 0; y < sh; y++) {
                for (int x = 0; x < sw; x++) {
                    int idx = y * sw + x;
                    BYTE r = pixels[idx].rgbRed;
                    BYTE g = pixels[idx].rgbGreen;
                    BYTE b = pixels[idx].rgbBlue;

                    pixels[idx].rgbRed = g + (x ^ y);
                    pixels[idx].rgbGreen = b + static_cast<BYTE>(totalElapsed * 60);
                    pixels[idx].rgbBlue = r - (x ^ y);
                }
            }

            BitBlt(hdc, 0, 0, sw, sh, memDC, 0, 0, SRCCOPY);

            DeleteObject(hDIB);
            DeleteDC(memDC);
        }

        // --- NOVÁ FÁZE: Diagonální neonové vlny (přesně jako na videu) ---
        else if (elapsed < 60.0f) { // (Nezapomeň nahoře zvětšit cycleDuration např. na 50.0f!)
            HDC memDC = CreateCompatibleDC(hdc);
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = sw;
            bmi.bmiHeader.biHeight = -sh;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            RGBQUAD* pixels = nullptr;
            HBITMAP hDIB = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
            SelectObject(memDC, hDIB);

            // Stažení aktuální obrazovky do paměti
            BitBlt(memDC, 0, 0, sw, sh, hdc, 0, 0, SRCCOPY);

            // Rychlost pohybu pruhů (čím vyšší číslo, tím rychleji to teče)
            int timeShift = static_cast<int>(totalElapsed * 150.0f);

            for (int y = 0; y < sh; y++) {
                for (int x = 0; x < sw; x++) {
                    int idx = y * sw + x;

                    BYTE r = pixels[idx].rgbRed;
                    BYTE g = pixels[idx].rgbGreen;
                    BYTE b = pixels[idx].rgbBlue;

                    // (x + y) vytvoří diagonálu. Dělením / 3 uděláme pruhy tlustší (jako na videu).
                    // Modulo 255 nám zajistí, že se vzor bude plynule opakovat.
                    int stripe = ((x + y) / 3 + timeShift) % 255;

                    // Míchání barev: 
                    // Bitový operátor '& 255' je superrychlý způsob, jak zajistit, že barva nepřeteče přes maximum 255
                    pixels[idx].rgbRed = (r + stripe) & 255;

                    // Posuneme zelenou o 128 hodnot, to vytvoří ten brutální fialovo/zelený kontrast!
                    pixels[idx].rgbGreen = (g + stripe + 128) & 255;

                    pixels[idx].rgbBlue = (b + stripe) & 255;
                }
            }

            // Vykreslení upravené obrazovky zpět
            BitBlt(hdc, 0, 0, sw, sh, memDC, 0, 0, SRCCOPY);

            DeleteObject(hDIB);
            DeleteDC(memDC);
            }

            // --- NOVÁ FÁZE: Náhodný podpis "Filasss666" po celé obrazovce ---
        else if (elapsed < 75.0f) { // Nezapomeň případně nahoře upravit cycleDuration (např. na 70.0f)

            // Nastavíme průhledné pozadí textu, aby nebyly kolem písma černé čtverce
                SetBkMode(hdc, TRANSPARENT);

                // V každém kroku vykreslíme několik nápisů
                for (int i = 0; i < 6; i++) {
                    int x = rand() % sw;
                    int y = rand() % sh;
                    int fontSize = 24 + (rand() % 64); // Náhodná velikost písma (24px až 88px)
                    int angle = (rand() % 360) * 10;   // Náhodné natočení textu (0° až 360°)

                    // Náhodná pestrá barva
                    COLORREF color = RGB(rand() % 256, rand() % 256, rand() % 256);
                    SetTextColor(hdc, color);

                    // Vytvoření tučného fontu (můžeš změnit L"Impact" třeba na L"Arial" nebo L"Comic Sans MS")
                    HFONT hFont = CreateFontW(
                        fontSize, 0, angle, angle, FW_EXTRABOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact"
                    );

                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                    // Vykreslení tvého nicku
                    const wchar_t* text = L"Filasss666";
                    TextOutW(hdc, x, y, text, lstrlenW(text));

                    // Okamžitý úklid fontu z paměti
                    SelectObject(hdc, hOldFont);
                    DeleteObject(hFont);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
    }

    // Zastavení přehrávání hudby
    PlaySound(NULL, NULL, 0);

    // 4. Úklid obrazovky po vypnutí programu (vrátí vše do normálu)
    ReleaseDC(NULL, hdc);
    RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);

    return 0;
}