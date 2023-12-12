//***************************************************************************
// 
// 파일: main.c
// 
// 설명: 메인 함수
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/12/11
// 
//***************************************************************************

#include "precompiled.h"
#include "key_event.h"
#include "game.h"

#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC

HINSTANCE g_hinstance;
HWND g_hwnd;

HRESULT init_window(const size_t width, const size_t height);
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

int main(void)
{
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    //_CrtSetBreakAlloc(101);

    const int window_width = 800;
    const int window_height = 600;

    // 창 생성
    if (FAILED(init_window(window_width, window_height)))
    {
        return -1;
    }

    // 게임 객체 생성
    gp_game = (game_t*)malloc(sizeof(game_t));
    if (gp_game == NULL)
    {
        ASSERT(false, "Failed to malloc game");
        return -1;
    }

    // 게임 초기화
    if (!init_game())
    {
        ASSERT(false, "Failed to init game");
        SAFE_FREE(gp_game);
        return -1;
    }

    // 메인 루프
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 게임 루프
            if (is_running_game())
            {
                tick_game();
            }
        }
    }

    shutdown_game();
    SAFE_FREE(gp_game);

    _CrtDumpMemoryLeaks();
    return (int)msg.wParam;
}

HRESULT init_window(const size_t width, const size_t height)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wnd_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandleW(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"safe99";
    wcex.hIconSm = NULL;
    if (!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }

    // Create window
    g_hinstance = wcex.hInstance;
    RECT rc = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindow(wcex.lpszClassName, L"safe99 - sokoban",
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                          CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, wcex.hInstance,
                          NULL);
    if (!g_hwnd)
    {
        return E_FAIL;
    }

    GetClientRect(g_hwnd, &rc);
    ClientToScreen(g_hwnd, (POINT*)&rc.left);
    ClientToScreen(g_hwnd, (POINT*)&rc.right);

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);

    return S_OK;
}

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    uint8_t key;
    int x;
    int y;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
        down_left_mouse();
        break;
    case WM_LBUTTONUP:
        up_left_mouse();
        break;
    case WM_RBUTTONDOWN:
        down_right_mouse();
        break;
    case WM_RBUTTONUP:
        up_right_mouse();
        break;
    case WM_MOUSEMOVE:
        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);
        move_mouse(x, y);
        break;

    case WM_KEYDOWN:
        key = (uint8_t)wParam;
        key_down(key);
        break;
    case WM_KEYUP:
        key = (uint8_t)wParam;
        key_up(key);
        break;

    case WM_SIZE:
        if (gp_game != NULL)
        {
            update_window_size_game();
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}