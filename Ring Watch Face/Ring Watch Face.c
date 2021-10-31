/*
Source File: Ring Watch Face.c
Last Update: 2021/10/31

This project is hosted on https://github.com/Hydr10n/Ring-Watch-Face
Copyright (C) Hydr10n@GitHub. All Rights Reserved.
*/

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#include <Windows.h>
#include <windowsx.h>
#include <math.h>

#define FPS 30

#define MAIN_WINDOW_DIAMETER 500

#define Scale(Pixels, DPI) MulDiv((int)(Pixels), (int)(DPI), USER_DEFAULT_SCREEN_DPI)

int iDPI = USER_DEFAULT_SCREEN_DPI;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void WINAPI DrawWatchFace(HDC hDC, const PRECT pRect, const PSYSTEMTIME pTime);
void WINAPI DrawTime(HDC hDC, const PRECT pRect, const PSYSTEMTIME pTime);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	HDC hDC = GetDC(NULL);
	if (hDC != NULL) {
		SetProcessDPIAware();
		iDPI = GetDeviceCaps(hDC, LOGPIXELSY);
		ReleaseDC(NULL, hDC);
	}

	const int iDiameter = Scale(MAIN_WINDOW_DIAMETER, iDPI);

	WNDCLASS wndClass = { 0 };
	wndClass.lpszClassName = TEXT("Watch Face");
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = GetStockBrush(BLACK_BRUSH);
	RegisterClass(&wndClass);

	HWND hWnd = CreateWindowEx(WS_EX_LAYERED,
		wndClass.lpszClassName, TEXT("Ring Watch Face"),
		WS_POPUP,
		(GetSystemMetrics(SM_CXSCREEN) - iDiameter) / 2, (GetSystemMetrics(SM_CYSCREEN) - iDiameter) / 2, iDiameter, iDiameter,
		NULL, NULL, hInstance, NULL);

	SetLayeredWindowAttributes(hWnd, 0, 0, LWA_COLORKEY);

	ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: SetTimer(hWnd, 1, (WPARAM)(1000.0 / FPS), NULL); break;

	case WM_LBUTTONDOWN: SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0); break;

	case WM_PAINT: {
		RECT rc;
		GetClientRect(hWnd, &rc);

		SYSTEMTIME time;
		GetLocalTime(&time);

		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps), hCompatibleDC = CreateCompatibleDC(NULL);
		HBITMAP hBitmap = SelectBitmap(hCompatibleDC, CreateCompatibleBitmap(hDC, rc.right, rc.bottom));
		HFONT hFont = SelectFont(hCompatibleDC,
			CreateFontW(Scale(-50 * rc.bottom / Scale(500, iDPI), iDPI),
				0, 0, 0, FW_NORMAL,
				FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
				TEXT("Segoe UI")));

		SetBkMode(hCompatibleDC, TRANSPARENT);
		SetTextColor(hCompatibleDC, RGB(0xf0, 0xf0, 0xf0));
		DrawWatchFace(hCompatibleDC, &rc, &time);
		DrawTime(hCompatibleDC, &rc, &time);
		BitBlt(hDC, 0, 0, rc.right, rc.bottom, hCompatibleDC, 0, 0, SRCCOPY);

		DeleteFont(SelectFont(hCompatibleDC, hFont));
		DeleteBitmap(SelectBitmap(hCompatibleDC, hBitmap));
		DeleteDC(hCompatibleDC);
		EndPaint(hWnd, &ps);
	}	break;

	case WM_TIMER: InvalidateRect(hWnd, NULL, FALSE); break;

	case WM_DESTROY: PostQuitMessage(ERROR_SUCCESS); break;

	default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void WINAPI DrawWatchFace(HDC hDC, const PRECT pRect, const PSYSTEMTIME pTime) {
	const SIZE size = { pRect->right - pRect->left, pRect->bottom - pRect->top };

	TRIVERTEX vertices[] = {
		{ (LONG)(size.cx * 0.2), (LONG)(size.cy * 0.2), 0x00 << 8, 0x04 << 8, 0x28 << 8 },
		{ (LONG)(size.cx * 0.8), (LONG)(size.cy * 0.8), 0x00 << 8, 0x4e << 8, 0x92 << 8 }
	};
	GdiGradientFill(hDC, vertices, ARRAYSIZE(vertices), &(GRADIENT_RECT){ 0, 1 }, 1, GRADIENT_FILL_RECT_V);

	const double Pi = 3.14159265,
		radius = min(size.cx, size.cy) / 2.0,
		originX = size.cx / 2.0, originY = size.cy / 2.0,
		second = pTime->wSecond + pTime->wMilliseconds / 1000.0,
		minute = pTime->wMinute + second / 60,
		hour = pTime->wHour + minute / 60;

	const struct {
		double Radians;
		COLORREF Color;
	} rings[] = {
		{ Pi / 2 - 2 * Pi * second / 60, RGB(0x17, 0x2e, 0x7c) },
		{ Pi / 2 - 2 * Pi * minute / 60, RGB(0x21, 0x40, 0x9a) },
		{ Pi / 2 - 2 * Pi * hour / 12, RGB(0x04, 0x65, 0xb2) }
	};

	for (int i = 0; i < ARRAYSIZE(rings); i++) {
		const double radius1 = radius * (1 - i * 0.15), radius2 = radius1 - radius * 0.15;

		{
			HRGN hRgn1 = CreateEllipticRgn((int)(originX - radius1), (int)(originY - radius1), (int)(originX + radius1), (int)(originY + radius1)),
				hRgn2 = CreateEllipticRgn((int)(originX - radius2), (int)(originY - radius2), (int)(originX + radius2), (int)(originY + radius2));
			HBRUSH hBrush = CreateSolidBrush(rings[i].Color);

			CombineRgn(hRgn1, hRgn1, hRgn2, RGN_XOR);
			FillRgn(hDC, hRgn1, hBrush);

			DeleteBrush(hBrush);
			DeleteRgn(hRgn2);
			DeleteRgn(hRgn1);
		}

		{
			const COLORREF color = RGB(0xe0, 0xe0, 0xe0);
			HPEN hPen = SelectPen(hDC, CreatePen(PS_SOLID, 1, color));
			HBRUSH hBrush = SelectBrush(hDC, CreateSolidBrush(color));

			const double radiusSum = (radius1 + radius2) / 2, radiusDiff = (radius1 - radius2) / 2 * 0.8,
				pointX = originX + radiusSum * cos(rings[i].Radians), pointY = originY - radiusSum * sin(rings[i].Radians);
			Ellipse(hDC, (int)(pointX - radiusDiff), (int)(pointY - radiusDiff), (int)(pointX + radiusDiff), (int)(pointY + radiusDiff));

			DeleteBrush(SelectBrush(hDC, hBrush));
			DeletePen(SelectPen(hDC, hPen));
		}
	}
}

void WINAPI DrawTime(HDC hDC, const PRECT pRect, const PSYSTEMTIME pTime) {
	TCHAR szTime[9];
	wsprintf(szTime, TEXT("%02hu:%02hu:%02hu"), pTime->wHour, pTime->wMinute, pTime->wSecond);
	DrawText(hDC, szTime, -1, pRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}
