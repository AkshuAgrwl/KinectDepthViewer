#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

#include <NuiApi.h>

#include <iostream>

#define WIN_WIDTH 640
#define WIN_HEIGHT 480

HANDLE depthStream;
HANDLE rgbStream;
bool g_bRunning = true;

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        g_bRunning = false;
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

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
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        NuiShutdown();
    } catch (const std::exception &e) {
        std::wstring ws(e.what(), e.what() + strlen(e.what()));
        MessageBox(hwnd, ws.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
