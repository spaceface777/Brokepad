// #define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "res/res.h"

#define WIN_NAME "Brokepad"
#define TITLE_SEP " · "

typedef struct Line {
	char* str;
	int   pos;
} Line;

Line* lines = 0;
int nr_lines = 1;

int row=0, col=0;

#define window_width 960
#define window_height 730

#define margin_left 6
#define margin_top  4
#define content_width (window_width - margin_left*2)
#define content_height (window_height - margin_top*2)

int max_row = 0, max_col = 0;

int window_left=0, window_top=0;

HFONT hFont, hFontBold;
const int font_width = 13, font_height = 24;

int handle_events = 1;

char* cur_filename = "Untitled";
int file_modified = 0;
void update_title(HWND hwnd, int state) {
	if (state && !file_modified) {
		file_modified = 1;
		wchar_t buf[256];
		swprintf(buf, 256, L"*%hs - Brokepad", cur_filename);
		SetWindowTextW(hwnd, buf);
	} else if (!state) {
		file_modified = 0;
		wchar_t buf[256];
		swprintf(buf, 256, L"%hs - Brokepad", cur_filename);
		SetWindowTextW(hwnd, buf);
	}
}

void move_window(HWND hwnd) {
	double left = window_left - col*(font_width);
	double top = window_top - row*(font_height);

	RECT wRect;
	GetWindowRect(hwnd, &wRect);

	const double dx = left - wRect.left;
	const double dy = top - wRect.top;
	const double dist = sqrt(dx*dx + dy*dy);

	if (dist < 50.0) {
		SetWindowPos(hwnd, 0, (int)left, (int)top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		return;
	}

	int _old_handle_events = handle_events;
	handle_events = 0;

	double fps = GetDeviceCaps(GetDC(0), VREFRESH);
	const double x_movement_per_frame = dx/(fps*0.8);
	const double y_movement_per_frame = dy/(fps*0.8);
	const double movement_per_frame = sqrt(x_movement_per_frame*x_movement_per_frame + y_movement_per_frame*y_movement_per_frame);

	left = wRect.left, top = wRect.top;

	for (double i = 0; i < dist; i += movement_per_frame) {
		left += x_movement_per_frame;
		top += y_movement_per_frame;

		SetWindowPos(hwnd, 0, (int)(left+0.5), (int)(top+0.5), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		Sleep(1000/fps);
	}
	handle_events = _old_handle_events;
}

void update_window_pos(HWND hwnd) {
	RECT wRect;
	GetWindowRect(hwnd, &wRect);
	window_left = wRect.left + col*(font_width);
	window_top = wRect.top + row*(font_height);
}

extern const char _binary_typewriter_start_wav_start;
extern const char _binary_typewriter_start_wav_end;
#define typewriter_start_wav		(&_binary_typewriter_start_wav_start)
#define typewriter_start_wav_len	(&_binary_typewriter_start_wav_end - &_binary_typewriter_start_wav_start)

extern const char _binary_typewriter_end_wav_start;
extern const char _binary_typewriter_end_wav_end;
#define typewriter_end_wav		(&_binary_typewriter_end_wav_start)
#define typewriter_end_wav_len	(&_binary_typewriter_end_wav_end - &_binary_typewriter_end_wav_start)

extern const char _binary_typewriter_return_wav_start;
extern const char _binary_typewriter_return_wav_end;
#define typewriter_return_wav		(&_binary_typewriter_return_wav_start)
#define typewriter_return_wav_len	(&_binary_typewriter_return_wav_end - &_binary_typewriter_return_wav_start)

extern const char _binary_crumpled_paper_wav_start;
extern const char _binary_crumpled_paper_wav_end;
#define crumpled_paper_wav		(&_binary_crumpled_paper_wav_start)
#define crumpled_paper_wav_len	(&_binary_crumpled_paper_wav_end - &_binary_crumpled_paper_wav_start)

extern const char _binary_Kingthings_Trypewriter_2_ttf_start;
extern const char _binary_Kingthings_Trypewriter_2_ttf_end;
#define Kingthings_Trypewriter_ttf		(&_binary_Kingthings_Trypewriter_2_ttf_start)
#define Kingthings_Trypewriter_ttf_len	(&_binary_Kingthings_Trypewriter_2_ttf_end - &_binary_Kingthings_Trypewriter_2_ttf_start)

typedef enum Sound {
	TYPEWRITER_START = 1,
	TYPEWRITER_END,
	TYPEWRITER_RETURN,
	CRUMPLED_PAPER
} Sound;

void play_sound(Sound s) {
	switch (s) {
		case TYPEWRITER_START:						PlaySound(typewriter_start_wav, 0, 0x15);	break;
		case TYPEWRITER_END:						PlaySound(typewriter_end_wav, 0, 0x15);		break;
		case TYPEWRITER_RETURN: PlaySound(0, 0, 0); PlaySound(typewriter_return_wav, 0, 0x15);	break;
		case CRUMPLED_PAPER:	PlaySound(0, 0, 0); PlaySound(crumpled_paper_wav, 0, 0x15);		break;
	}
}

int load_file(const char* path) {
	for (int i = 0; i <= max_row; i++) {
		memset(lines[i].str, 0, max_col);
		lines[i].pos = 0;
	}
	row = 0, col = 0;

	FILE* f = fopen(path, "rb");

	const int m = max_col+1;
	char line_buf[128];
	memset(line_buf, 0, m+1);
	nr_lines = 0;
	while (fgets(line_buf, m, f) && nr_lines < max_row) {
		int slen = strlen(line_buf);
		if (line_buf[slen-1] == '\n') {
			line_buf[slen-1] = 0;
			slen--;
		}
		char* s = lines[nr_lines].str;
		memcpy(s, line_buf, slen+1);
		nr_lines++;
		memset(line_buf, 0, m+1);

		int c;
		if (slen == max_col && (c = fgetc(f), c != '\n')) ungetc(c, f);
	}
	nr_lines++;

	fclose(f);
	return 0;
}

int save_file(const char* path) {
	FILE* f = fopen(path, "wb");
	for (int i=0; i<nr_lines; i++) {
		Line line = lines[i];
		char* str = line.str;
		if (str) {
			fputs(str, f);
			fputc('\n', f);
		}
	}
	fclose(f);
	return 0;
}

void CTRL_N(HWND hwnd) {
	play_sound(CRUMPLED_PAPER);
	for (int i = 0; i <= max_row; i++) {
		memset(lines[i].str, 0, max_col);
		lines[i].pos = 0;
	}
	row = 0, col = 0;
	update_window_pos(hwnd);
	cur_filename = "Untitled";
	update_title(hwnd, 0);
	UpdateWindow(hwnd);
}

void CTRL_SHIFT_N() {
	STARTUPINFO si = {0};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

#define new_OPENFILENAMEA(filename) \
	{0}; \
	Ofn.lStructSize = sizeof(OPENFILENAME); \
	Ofn.hwndOwner = hwnd; \
	Ofn.lpstrFilter = "Text Document (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0"; \
	Ofn.lpstrDefExt = "txt"; \
	Ofn.lpstrFile = filename; \
	Ofn.nMaxFile = sizeof(filename); \
	Ofn.Flags = OFN_OVERWRITEPROMPT; \
	Ofn.lpstrTitle = "data.txt"; \

void CTRL_O(HWND hwnd) {
	char filename[1024] = "";
	OPENFILENAMEA Ofn = new_OPENFILENAMEA(filename);
	GetOpenFileNameA(&Ofn);

	load_file(Ofn.lpstrFile);

	char fname[256];
	char ext[256];
	_splitpath(filename, 0, 0, fname, ext);
	cur_filename = calloc(strlen(fname) + strlen(ext) + 2, 1);
	sprintf(cur_filename, "%s%s", fname, ext);

	update_title(hwnd, 0);

	row = 0, col = 0;
	update_window_pos(hwnd);
}

void CTRL_SHIFT_S(HWND hwnd);
void CTRL_S(HWND hwnd, char* filename) {
	if (strcmp(filename, "Untitled") == 0) {
		CTRL_SHIFT_S(hwnd);
		return;
	}
	save_file(filename);

	char fname[256];
	char ext[256];
	_splitpath(filename, 0, 0, fname, ext);
	cur_filename = calloc(strlen(fname) + strlen(ext) + 2, 1);
	sprintf(cur_filename, "%s%s", fname, ext);

	update_title(hwnd, 0);
}

void CTRL_SHIFT_S(HWND hwnd) {
	char* filename;
	char buf[1024] = {0};
	OPENFILENAMEA Ofn = new_OPENFILENAMEA(buf);
	GetSaveFileNameA(&Ofn);
	filename = Ofn.lpstrFile;
	if (!filename || !*filename) return;

	CTRL_S(hwnd, filename);
}
#undef new_OPENFILENAMEA


RECT window_rect;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
	case WM_EXITSIZEMOVE: {
		update_window_pos(hwnd);

		break;
	}

	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		char filename[MAX_PATH];
		DragQueryFile(hDrop, 0, filename, MAX_PATH);
		DragFinish(hDrop);

		load_file(filename);

		char fname[256];
		char ext[256];
		_splitpath(filename, 0, 0, fname, ext);
		cur_filename = calloc(strlen(fname) + strlen(ext) + 2, 1);
		sprintf(cur_filename, "%s%s", fname, ext);

		update_title(hwnd, 0);

		row = 0, col = 0;
		update_window_pos(hwnd);

		break;
	}

	case WM_CLOSE: {
		if (file_modified) {
			FlashWindow(hwnd, TRUE);
			char buf[1024];
			sprintf(buf, "Do you want to save changes to %s?", cur_filename);
			int ret = MessageBox(hwnd, buf, WIN_NAME, MB_YESNOCANCEL);
			if (ret == IDYES) {
				CTRL_S(hwnd, cur_filename);
				return 0;
			} else if (ret == IDCANCEL) {
				return 0;
			}
		}

		break;
	}

	case WM_DESTROY: {
		PostQuitMessage(0);

		break;
	}

	case WM_COMMAND: {
		switch (wParam) {
			case SHORTCUT_NEW:        CTRL_N(hwnd); break;
			case SHORTCUT_NEW_WINDOW: CTRL_SHIFT_N(hwnd); break;
			case SHORTCUT_OPEN:       CTRL_O(hwnd); break;
			case SHORTCUT_SAVE:       CTRL_S(hwnd, cur_filename); break;
			case SHORTCUT_SAVE_AS:    CTRL_SHIFT_S(hwnd); break;
			case SHORTCUT_EXIT:       DestroyWindow(hwnd); break;
		}

		break;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps = (PAINTSTRUCT){0};
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)COLOR_WINDOW);

		RECT rect = window_rect;
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));

		SetBkMode(hdc, TRANSPARENT);


		if (!lines) break;
		int e = nr_lines < max_row ? nr_lines : max_row;
		for (int i=0; i<e; i++) {
			Line line = lines[i];
			char* str = line.str;

			if (i < 6) {
				SelectObject(hdc, hFontBold);
				SetTextColor(hdc, 0x121212 * i);
			} else {
				SelectObject(hdc, hFont);
				SetTextColor(hdc, 0x090909 * (i-6));
			}

			TextOut(hdc, rect.left, rect.top, str, strlen(str));
			rect.top += font_height;
		}
		if (col < max_col) {
			rect.left = margin_left + col*(font_width);
			rect.right = rect.left + font_width;
			rect.top = margin_top + (row+1)*font_height;
			rect.bottom = rect.top + 2;

			HBRUSH b = CreateSolidBrush(0x888888);
			FillRect(hdc, &rect, b);
			DeleteObject(b);
		}
		EndPaint(hwnd, &ps);

		break;
	}

	case WM_KEYDOWN: {
		// repeated keypress - ignore this event
		if (lParam & (1<<30)) break;
		if (wParam >= 32 && wParam <= 126 && !(GetKeyState(VK_CONTROL) & 0x8000)) play_sound(TYPEWRITER_START);

		break;
	}

	case WM_KEYUP: {
		if (wParam >= 32 && wParam <= 126 && !(GetKeyState(VK_CONTROL) & 0x8000)) play_sound(TYPEWRITER_END);

		switch(wParam) {
			// case VK_ESCAPE: {
			//     DestroyWindow(hwnd);
			//     exit(0);
			//     break;
			// }
			case VK_BACK: {
				Line* line = lines + row;
				if (line->pos > 0) {
					play_sound(TYPEWRITER_END);
					--line->pos;
					--col;
					move_window(hwnd);
					RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE);
				}
				break;
			}
		}

		if (GetKeyState(VK_CONTROL) & 0x8000) {
			switch(wParam) {
				case 'N': {
					if (GetKeyState(VK_SHIFT) & 0x8000) {
						CTRL_SHIFT_N(hwnd);
					} else {
						CTRL_N(hwnd);
					}
					break;
				}

				case 'S': {
					if (GetKeyState(VK_SHIFT) & 0x8000) {
						CTRL_SHIFT_S(hwnd);
					} else {
						CTRL_S(hwnd, cur_filename);
					}
					break;
				}
				case 'O': {
					CTRL_O(hwnd);
					break;
				}
				#ifndef NDEBUG
				case 'J': {
					// shhh
					if (GetKeyState(VK_SHIFT) & 0x8000)
						SetWindowPos(hwnd, 0, 10, 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
				#endif
			}
		}

		RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE);
		break;
	}

	case WM_CHAR: {
		if (lParam & (1<<30)) {
			// repeated keypress - ignore this event
			break;
		}
		if (wParam == VK_RETURN) {
			if (row >= max_row) break;
			int old_col = col;
			col = 0;
			row++;
			++nr_lines;
			if (old_col != 0) play_sound(TYPEWRITER_RETURN);
			move_window(hwnd);
		}

		if (wParam < 32 || wParam > 126) break;

		Line* line = lines + row;

		if (line->pos < max_col) {
			update_title(hwnd, 1);

			line->str[line->pos] = wParam;
			line->pos++;
			col++;
			move_window(hwnd);
		}

		RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE);

		break;
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void init_font(HWND hwnd) {
	DWORD __num_fonts = 1;
	AddFontMemResourceEx((void*)Kingthings_Trypewriter_ttf, Kingthings_Trypewriter_ttf_len, 0, &__num_fonts);

	(void)hwnd;
	LOGFONT tmp = {
		.lfFaceName = "Kingthings Trypewriter 2",
		.lfHeight = font_height+1,
		.lfQuality = ANTIALIASED_QUALITY,
	};
	hFont = CreateFontIndirectA(&tmp);

	tmp.lfWeight = FW_BOLD;
	tmp.lfHeight = font_height-1;
	hFontBold = CreateFontIndirectA(&tmp);

	max_col = content_width / font_width;
	max_row = content_height / font_height - 3;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;

	puts("started");

	HICON hIcon = (HICON)LoadImageA(hInstance, MAKEINTRESOURCEA(IDI_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = hIcon;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = MAKEINTRESOURCEA(IDI_MENU);
	wc.lpszClassName = WIN_NAME;
	wc.hIconSm       = hIcon;

	if(!RegisterClassEx(&wc)) {
		MessageBox(0, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	HWND hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE | WS_EX_COMPOSITED | WS_EX_ACCEPTFILES,
		WIN_NAME,
		WIN_NAME,
		(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
		0, 0, hInstance, 0);

	if (!hwnd) {
		MessageBox(0, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	SetWindowTextW(hwnd, L"Untitled - Brokepad");

	ShowWindow(hwnd, nCmdShow);

	update_window_pos(hwnd);
	init_font(hwnd);

	GetClientRect (hwnd, &window_rect);
	window_rect.left += margin_left;
	window_rect.right -= margin_left;
	window_rect.top += margin_top;
	window_rect.bottom -= margin_top;

	UpdateWindow(hwnd);

	lines = calloc(max_row+1, sizeof(Line));
	for (int i = 0; i <= max_row; i++) {
		lines[i].str = calloc(max_col+1, sizeof(char));
	}

	MSG Msg;
	while(GetMessage(&Msg, hwnd, 0, 0) > 0 && handle_events) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
