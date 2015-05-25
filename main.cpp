#ifndef UNICODE
#define UNICODE
#endif
#pragma comment(lib,"glew32s")
#define GLEW_STATIC

#include<windows.h>
#include<richedit.h>
#include<math.h>
#include<GL/glew.h>
#include<GL/glut.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_ASPECT (WINDOW_HEIGHT.0f/WINDOW_WIDTH.0f)
#define ID_EDITSHOW 1000
#define ID_SELECTALL 1001
#define WM_CREATED WM_APP
#define WM_SETALPHA (WM_APP+1)
#define ANIMATION_STEP 32
#define SHOWEDITALPHA 192
#define HIDEDITALPHA 64

HDC hDC;
HGLRC hRC;
BOOL active;
GLuint program;
GLuint vao;
GLuint vbo;
WNDPROC EditWndProc;
const TCHAR szClassName[] = TEXT("Window");
const GLfloat position[][2] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
const int vertices = sizeof position / sizeof position[0];
const GLchar vsrc[] = "in vec4 position;void main(void){gl_Position = position;}";

inline GLint GetShaderInfoLog(GLuint shader)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == 0) OutputDebugString(TEXT("Compile Error\n"));
	GLsizei bufSize;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufSize);
	if (bufSize > 1)
	{
		LPSTR infoLog = (LPSTR)GlobalAlloc(0, bufSize);
		GLsizei length;
		glGetShaderInfoLog(shader, bufSize, &length, infoLog);
		OutputDebugStringA(infoLog);
		GlobalFree(infoLog);
	}
	return status;
}

inline GLint GetProgramInfoLog(GLuint program)
{
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == 0) OutputDebugString(TEXT("Link Error\n"));
	GLsizei bufSize;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufSize);
	if (bufSize > 1)
	{
		LPSTR infoLog = (LPSTR)GlobalAlloc(0, bufSize);
		GLsizei length;
		glGetProgramInfoLog(program, bufSize, &length, infoLog);
		OutputDebugStringA(infoLog);
		GlobalFree(infoLog);
	}
	return status;
}

inline GLuint CreateProgram(LPCSTR vsrc, LPCSTR fsrc)
{
	const GLuint vobj = glCreateShader(GL_VERTEX_SHADER);
	if (!vobj) return 0;
	glShaderSource(vobj, 1, &vsrc, 0);
	glCompileShader(vobj);
	if (GetShaderInfoLog(vobj) == 0)
	{
		glDeleteShader(vobj);
		return 0;
	}
	const GLuint fobj = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fobj)
	{
		glDeleteShader(vobj);
		return 0;
	}
	glShaderSource(fobj, 1, &fsrc, 0);
	glCompileShader(fobj);
	if (GetShaderInfoLog(fobj) == 0)
	{
		glDeleteShader(vobj);
		glDeleteShader(fobj);
		return 0;
	}
	GLuint program = glCreateProgram();
	if (program)
	{
		glAttachShader(program, vobj);
		glAttachShader(program, fobj);
		glLinkProgram(program);
		if (GetProgramInfoLog(program) == 0)
		{
			glDetachShader(program, fobj);
			glDetachShader(program, vobj);
			glDeleteProgram(program);
			program = 0;
		}
	}
	glDeleteShader(vobj);
	glDeleteShader(fobj);
	return program;
}

inline BOOL InitGL(GLvoid)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 *
		vertices, position, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return TRUE;
}

inline VOID DrawGLScene(GLvoid)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	const GLint time = glGetUniformLocation(program, "time");
	glUniform1f(time, GetTickCount() / 1000.0f);
	glBindVertexArray(vao);
	glDrawArrays(GL_QUADS, 0, vertices);
	glBindVertexArray(0);
	glUseProgram(0);
	glFlush();
}

inline double easeOutExpo(double t, double b, double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bHover;
	static BYTE dwAlphaEnd;
	static BYTE dwAlphaStart;
	static BYTE dwAlphaStep;
	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) return SendMessage(GetParent(hWnd), WM_SYSCOMMAND, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		if (!bHover)
		{
			if (hWnd != GetFocus())	SetFocus(hWnd);
			bHover = TRUE;
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, 0, TRUE);
			PostMessage(hWnd, WM_SETALPHA, SHOWEDITALPHA, 0);
		}
		break;
	case WM_MOUSELEAVE:
		bHover = FALSE;
		PostMessage(hWnd, WM_SETALPHA, HIDEDITALPHA, 0);
		break;
	case WM_SETALPHA:
		KillTimer(hWnd, 0x1234);
		GetLayeredWindowAttributes(hWnd, 0, &dwAlphaStart, 0);
		dwAlphaEnd = wParam;
		dwAlphaStep = 0;
		SetTimer(hWnd, 0x1234, 1, 0);
		return 0;
	case WM_TIMER:
		if (dwAlphaStep > ANIMATION_STEP || dwAlphaStart == dwAlphaEnd)
		{
			KillTimer(hWnd, 0x1234);
			SetLayeredWindowAttributes(hWnd, 0, dwAlphaEnd, LWA_ALPHA);
		}
		else
		{
			dwAlphaStep++;
			const BYTE bByte = (dwAlphaStart < dwAlphaEnd) ? easeOutExpo(dwAlphaStep, dwAlphaStart, dwAlphaEnd - dwAlphaStart, ANIMATION_STEP) :
				dwAlphaStart + dwAlphaEnd - easeOutExpo(dwAlphaStep, dwAlphaEnd, dwAlphaStart - dwAlphaEnd, ANIMATION_STEP);
			SetLayeredWindowAttributes(hWnd, 0, bByte, LWA_ALPHA);
		}
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);
		break;
	}
	return CallWindowProc(EditWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	static GLuint PixelFormat;
	static HWND hEdit;
	static HFONT hFont;
	static HINSTANCE hRtLib;
	static BOOL bEditVisible = TRUE;
	switch (msg)
	{
	case WM_CREATE:
		hRtLib = LoadLibrary(TEXT("RICHED32"));
		hFont = CreateFont(24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Consolas"));
		hEdit = CreateWindowEx(WS_EX_LAYERED, RICHEDIT_CLASS, 0, WS_VISIBLE | WS_POPUP | WS_HSCROLL |
			WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
			0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit, EM_SETTEXTMODE, TM_PLAINTEXT, 0);
		SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
		EditWndProc = (WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC, (LONG)EditProc);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		if (!(hDC = GetDC(hWnd)) ||
			!(PixelFormat = ChoosePixelFormat(hDC, &pfd)) ||
			!SetPixelFormat(hDC, PixelFormat, &pfd) ||
			!(hRC = wglCreateContext(hDC)) ||
			!wglMakeCurrent(hDC, hRC) ||
			glewInit() != GLEW_OK ||
			!InitGL()) return -1;
		SetWindowText(hEdit, TEXT("uniform float time;\r\nvoid main()\r\n{\r\n\tgl_FragColor = vec4(sin(time));\r\n}"));
		PostMessage(hWnd, WM_CREATED, 0, 0);
		break;
	case WM_CREATED:
		SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(0, EN_CHANGE), (long)hEdit);
		SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM)(SendMessage(hEdit, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE));
		SetFocus(hEdit);
		PostMessage(hEdit, WM_SETALPHA, SHOWEDITALPHA, 0);
		break;
	case WM_MOVE:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		ClientToScreen(hWnd, (LPPOINT)&rect.left);
		ClientToScreen(hWnd, (LPPOINT)&rect.right);
		MoveWindow(hEdit, rect.left + 16, rect.top + 16, rect.right - rect.left - 32, rect.bottom - rect.top - 32, 1);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 0:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				const DWORD dwSize = GetWindowTextLengthA(hEdit);
				if (!dwSize) return 0;
				LPSTR lpszText = (LPSTR)GlobalAlloc(0, (dwSize + 1)*sizeof(CHAR));
				if (!lpszText) return 0;
				GetWindowTextA(hEdit, lpszText, dwSize + 1);
				const GLuint newProgram = CreateProgram(vsrc, lpszText);
				if (newProgram)
				{
					if (program) glDeleteProgram(program);
					program = newProgram;
					SetWindowText(hWnd, TEXT("フラグメントシェーダ [コンパイル成功]"));
				}
				else
				{
					SetWindowText(hWnd, TEXT("フラグメントシェーダ [コンパイル失敗]"));
				}
				GlobalFree(lpszText);
			}
			break;
		case ID_EDITSHOW:
			ShowWindow(hEdit, IsWindowVisible(hEdit) ? SW_HIDE : SW_NORMAL);
			break;
		case ID_SELECTALL:
			if (IsWindowVisible(hEdit))
			{
				SendMessage(hEdit, EM_SETSEL, 0, -1);
				SetFocus(hEdit);
			}
			break;
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		SendMessage(hWnd, WM_COMMAND, ID_EDITSHOW, 0);
		break;
	case WM_ACTIVATE:
		active = !HIWORD(wParam);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		if (program) glDeleteProgram(program);
		if (vbo) glDeleteBuffers(1, &vbo);
		if (vao) glDeleteVertexArrays(1, &vao);
		if (hRC)
		{
			wglMakeCurrent(0, 0);
			wglDeleteContext(hRC);
		}
		if (hDC) ReleaseDC(hWnd, hDC);
		FreeLibrary(hRtLib);
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	const WNDCLASS wndclass = { 0, WndProc, 0, 0, hInstance, 0, LoadCursor(0, IDC_ARROW), 0, 0, szClassName };
	RegisterClass(&wndclass);
	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	const DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	AdjustWindowRect(&rect, dwStyle, 0);
	const HWND hWnd = CreateWindow(szClassName, 0, dwStyle, CW_USEDEFAULT, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY, VK_ESCAPE, ID_EDITSHOW }, { FVIRTKEY | FCONTROL, 'A', ID_SELECTALL } };
	const HACCEL hAccel = CreateAcceleratorTable(Accel, sizeof(Accel) / sizeof(ACCEL));
	BOOL done = 0;
	while (!done)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = TRUE;
			}
			else if (!TranslateAccelerator(hWnd, hAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (active)
		{
			DrawGLScene();
			SwapBuffers(hDC);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return msg.wParam;
}