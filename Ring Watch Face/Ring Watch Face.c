/*
Source File: Ring Watch Face.c
Last Update: 2020/11/08

This project is hosted on https://github.com/Hydr10n/Ring-Watch-Face
Copyright (C) Hydr10n@GitHub. All Rights Reserved.
*/

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#include <Windows.h>
#include <windowsx.h>
#include <math.h>

#define MAIN_WINDOW_DIAMETER 500
#define FPS 30

#define Scale(iPixels, iDPI) MulDiv(iPixels, iDPI, USER_DEFAULT_SCREEN_DPI)

int iDPI = USER_DEFAULT_SCREEN_DPI;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI DrawWatchFace(HDC hDC, const PRECT pRect, const PSYSTEMTIME pSystemTime);
BOOL WINAPI DrawTime(HDC hDC, const PRECT pRect, const PSYSTEMTIME pSystemTime);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	int iDiameter = MAIN_WINDOW_DIAMETER;
	HDC hDC = GetDC(NULL);
	if (hDC != NULL) {
		SetProcessDPIAware();
		iDPI = GetDeviceCaps(hDC, LOGPIXELSY);
		iDiameter = Scale(iDiameter, iDPI);
		ReleaseDC(NULL, hDC);
	}
	WNDCLASSW wndClass = { 0 };
	wndClass.lpszClassName = L"Watch Face";
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = GetStockBrush(BLACK_BRUSH);
	RegisterClassW(&wndClass);
	HWND hWnd = CreateWindowExW(WS_EX_LAYERED, wndClass.lpszClassName, L"Ring Watch Face",
		WS_POPUP,
		(GetSystemMetrics(SM_CXSCREEN) - iDiameter) / 2, (GetSystemMetrics(SM_CYSCREEN) - iDiameter) / 2, iDiameter, iDiameter,
		NULL, NULL, hInstance, NULL);
	SetLayeredWindowAttributes(hWnd, RGB(0x00, 0x00, 0x00), 0, LWA_COLORKEY);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: SetTimer(hWnd, 1, (WPARAM)(1000.0 / FPS), NULL); break;
	case WM_LBUTTONDOWN: SendMessageW(hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0); break;
	case WM_PAINT: {
		RECT rect;
		GetClientRect(hWnd, &rect);
		SYSTEMTIME localTime;
		GetLocalTime(&localTime);
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps), hDC_Memory = CreateCompatibleDC(NULL);
		HBITMAP hBitmap_Old = SelectBitmap(hDC_Memory, CreateCompatibleBitmap(hDC, rect.right, rect.bottom));
		HFONT hFont_Old = SelectFont(hDC_Memory, CreateFontW(-MulDiv((int)(40.0 * rect.bottom / Scale(500, iDPI)), iDPI, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI"));
		SetBkMode(hDC_Memory, TRANSPARENT);
		SetTextColor(hDC_Memory, RGB(0xf0, 0xf0, 0xf0));
		DrawWatchFace(hDC_Memory, &rect, &localTime);
		DrawTime(hDC_Memory, &rect, &localTime);
		BitBlt(hDC, 0, 0, (int)rect.right, (int)rect.bottom, hDC_Memory, 0, 0, SRCCOPY);
		DeleteFont(SelectFont(hDC_Memory, hFont_Old));
		DeleteBitmap(SelectBitmap(hDC_Memory, hBitmap_Old));
		DeleteDC(hDC_Memory);
		EndPaint(hWnd, &ps);
	}	break;
	case WM_TIMER: InvalidateRect(hWnd, NULL, FALSE); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	default: return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

BOOL WINAPI DrawWatchFace(HDC hDC, const PRECT pRect, const PSYSTEMTIME pSystemTime) {
	const BOOL ret = hDC != NULL && pRect != NULL && pSystemTime != NULL;
	if (ret) {
		const SIZE size = { pRect->right - pRect->left, pRect->bottom - pRect->top };
		const double Pi = 3.14159265,
			radius = min(size.cx, size.cy) / 2.0,
			originX = size.cx / 2.0, originY = size.cy / 2.0,
			second = pSystemTime->wSecond + pSystemTime->wMilliseconds / 1000.0,
			minute = pSystemTime->wMinute + second / 60,
			hour = pSystemTime->wHour + minute / 60;
		const struct {
			double radians;
			COLORREF color;
		} rings[] = {
			{ Pi / 2 - 2 * Pi * second / 60, RGB(0x17, 0x2e, 0x7c) },
			{ Pi / 2 - 2 * Pi * minute / 60, RGB(0x21, 0x40, 0x9a) },
			{ Pi / 2 - 2 * Pi * hour / 12, RGB(0x04, 0x65, 0xb2) }
		};
		HRGN hRgn1, hRgn2 = NULL;
		for (INT64 i = 0; i < ARRAYSIZE(rings); i++) {
			const double radius1 = radius * (1 - i * 0.15), radius2 = radius * (1 - (i + 1) * 0.15);
			hRgn1 = i ? hRgn2 : CreateEllipticRgn((int)(originX - radius1), (int)(originY - radius1), (int)(originX + radius1), (int)(originY + radius1));
			hRgn2 = CreateEllipticRgn((int)(originX - radius2), (int)(originY - radius2), (int)(originX + radius2), (int)(originY + radius2));
			if (!i) {
				SelectClipRgn(hDC, hRgn1);
				TRIVERTEX triVertices[] = {
					{ (LONG)(size.cx * 0.2), (LONG)(size.cy * 0.2), 0x00 << 8, 0x04 << 8, 0x28 << 8 },
					{ (LONG)(size.cx * 0.8), (LONG)(size.cy * 0.8), 0x00 << 8, 0x4e << 8, 0x92 << 8 }
				};
				GRADIENT_RECT gradientRect = { 0, 1 };
				GdiGradientFill(hDC, triVertices, ARRAYSIZE(triVertices), &gradientRect, 1, GRADIENT_FILL_RECT_V);
			}
			CombineRgn(hRgn1, hRgn1, hRgn2, RGN_XOR);
			HBRUSH hBrush = CreateSolidBrush(rings[i].color);
			FillRgn(hDC, hRgn1, hBrush);
			DeleteBrush(hBrush);
			DeleteRgn(hRgn1);
			HPEN hPen_Old = SelectPen(hDC, CreatePen(PS_SOLID, 1, RGB(0xe0, 0xe0, 0xe0)));
			HBRUSH hBrush_Old = SelectBrush(hDC, CreateSolidBrush(RGB(0xe0, 0xe0, 0xe0)));
			const double radiusSum = (radius1 + radius2) / 2, radiusDiff = (radius1 - radius2) / 2 * 0.8,
				pointX = originX + radiusSum * cos(rings[i].radians), pointY = originY + radiusSum * -sin(rings[i].radians);
			Ellipse(hDC, (int)(pointX - radiusDiff), (int)(pointY - radiusDiff), (int)(pointX + radiusDiff), (int)(pointY + radiusDiff));
			DeleteBrush(SelectBrush(hDC, hBrush_Old));
			DeletePen(SelectPen(hDC, hPen_Old));
		}
		DeleteRgn(hRgn2);
	}
	return ret;
}

BOOL WINAPI DrawTime(HDC hDC, const PRECT pRect, const PSYSTEMTIME pSystemTime) {
	const BOOL ret = hDC != NULL && pRect != NULL && pSystemTime != NULL;
	if (ret) {
		WCHAR szTime[9];
		wsprintfW(szTime, L"%02hu:%02hu:%02hu", pSystemTime->wHour, pSystemTime->wMinute, pSystemTime->wSecond);
		DrawTextW(hDC, szTime, -1, pRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	}
	return ret;
}