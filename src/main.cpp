#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <windowsx.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>

#include <iostream>
#include <sstream>

#define WIN_WIDTH 640
#define WIN_HEIGHT 480

HANDLE depthStream;
HANDLE rgbStream;
HBITMAP hBitmap = NULL;
bool g_bRunning = true;

struct DepthInfo {
    int x = 0;
    int y = 0;
    USHORT depth = 0;
    bool valid = false;
} g_depthInfo;

void initKinect() {
    if (FAILED(NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH |
                             NUI_INITIALIZE_FLAG_USES_COLOR))) {
        throw std::runtime_error("Failed to initialize Kinect");
        exit(EXIT_FAILURE);
    }

    if (FAILED(NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH,
                                  NUI_IMAGE_RESOLUTION_640x480, 0, 2, NULL,
                                  &depthStream))) {
        throw std::runtime_error("Failed to open depth stream");
    }

    if (FAILED(NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,
                                  NUI_IMAGE_RESOLUTION_640x480, 0, 2, NULL,
                                  &rgbStream))) {
        throw std::runtime_error("Failed to open color stream");
    }
}

void ShowDepthAtPos(HWND hwnd, int x, int y) {
    const NUI_IMAGE_FRAME *pImageFrame = nullptr;

    if (SUCCEEDED(NuiImageStreamGetNextFrame(depthStream, 0, &pImageFrame))) {
        NUI_LOCKED_RECT lockedRect = {0};
        pImageFrame->pFrameTexture->LockRect(0, &lockedRect, NULL, 0);

        if (lockedRect.Pitch != 0) {
            USHORT *pBuffer = (USHORT *)lockedRect.pBits;
            int index = y * WIN_WIDTH + x;

            if (index >= 0 && index < WIN_WIDTH * WIN_HEIGHT) {
                g_depthInfo.x = x;
                g_depthInfo.y = y;
                g_depthInfo.depth = NuiDepthPixelToDepth(pBuffer[index]);
                g_depthInfo.valid = true;

                InvalidateRect(hwnd, NULL, TRUE);
            } else {
                g_depthInfo.valid = false;
            }
        }

        pImageFrame->pFrameTexture->UnlockRect(0);
        NuiImageStreamReleaseFrame(depthStream, pImageFrame);
    }
}

void ProcessFrame(HWND hwnd) {
    const NUI_IMAGE_FRAME *pImageFrame = nullptr;

    if (SUCCEEDED(NuiImageStreamGetNextFrame(rgbStream, 0, &pImageFrame))) {
        NUI_LOCKED_RECT lockedRect = {0};
        pImageFrame->pFrameTexture->LockRect(0, &lockedRect, NULL, 0);

        if (lockedRect.Pitch != 0) {
            static BITMAPINFO bmi = {0};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = WIN_WIDTH;
            bmi.bmiHeader.biHeight = -WIN_HEIGHT;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            HDC hdc = GetDC(hwnd);

            if (hBitmap == NULL) {
                hBitmap =
                    CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
            }

            if (hBitmap) {
                BYTE *pBits = NULL;
                BITMAP bmp;
                GetObject(hBitmap, sizeof(bmp), &bmp);
                pBits = (BYTE *)bmp.bmBits;

                if (pBits) {
                    BYTE *pBufferRun = (BYTE *)lockedRect.pBits;

                    for (int y = 0; y < WIN_HEIGHT; y++) {
                        for (int x = 0; x < WIN_WIDTH; x++) {
                            pBits[0] = pBufferRun[0]; // R
                            pBits[1] = pBufferRun[1]; // G
                            pBits[2] = pBufferRun[2]; // B
                            pBits[3] = 0;             // Alpha

                            pBits += 4;
                            pBufferRun += 4;
                        }
                    }
                }
            }

            ReleaseDC(hwnd, hdc);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }

        pImageFrame->pFrameTexture->UnlockRect(0);
        NuiImageStreamReleaseFrame(rgbStream, pImageFrame);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
    int xPos, yPos;

    switch (uMsg) {
    case WM_DESTROY: {
        g_bRunning = false;
        PostQuitMessage(0);
        return 0;
    }

    case WM_MOUSEMOVE: {
        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);
        ShowDepthAtPos(hwnd, xPos, yPos);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (hBitmap) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

            BitBlt(hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOldBitmap);
            DeleteDC(hdcMem);
        }

        if (g_depthInfo.valid) {
            std::wstringstream ss;
            ss << L"Depth: " << g_depthInfo.depth << L" mm";

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            RECT textRect = {g_depthInfo.x - 5, g_depthInfo.y - 5,
                             g_depthInfo.x + 150, g_depthInfo.y + 20};
            FillRect(hdc, &textRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
            TextOut(hdc, g_depthInfo.x, g_depthInfo.y, ss.str().c_str(),
                    (int)ss.str().length());
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"KinectDepthViewerWinCls";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Failed to register window", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Kinect Depth Viewer", WS_OVERLAPPEDWINDOW,

        CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH + 16, WIN_HEIGHT + 39,

        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, L"Failed to create window", L"Error",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    try {
        initKinect();

        MSG msg = {};

        while (g_bRunning) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                switch (msg.message) {
                case WM_QUIT: {
                    g_bRunning = false;
                    break;
                }

                case WM_MOUSEMOVE:
                    UpdateWindow(hwnd);
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            ProcessFrame(hwnd);
            Sleep(16);
        }

        NuiShutdown();
    } catch (const std::exception &e) {
        std::wstring ws(e.what(), e.what() + strlen(e.what()));
        MessageBox(hwnd, ws.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
