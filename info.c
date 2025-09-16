#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <windows.h>
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ws2_32.lib")

#define p_vscprintf(fmt, args) _vscprintf(fmt, args)

/* Silly macro to avoid repetition */
#define APPEND_AND_FREE(info, func)              \
    do {                                         \
        char* tmp = func;                        \
        info = append_dynamic(info, "%s", tmp);  \
        free(tmp);                               \
    } while (0)

/* Helpers functions to format dynamic strings */
char* format_dynamic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = p_vscprintf(fmt, args);
    va_end(args);

    if (needed < 0) return NULL;

    char* buf = (char*)malloc(needed + 1);
    if (!buf) return NULL;

    va_start(args, fmt);
    vsnprintf(buf, needed + 1, fmt, args);
    va_end(args);

    return buf;
}
 
char* append_dynamic(char* base, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = p_vscprintf(fmt, args);
    va_end(args);

    if (needed < 0) return NULL;

    size_t base_len = base ? strlen(base) : 0;
    char* buf = (char*)realloc(base, base_len + needed + 1);
    if (!buf) {
        free(base);
        return NULL;
    }

    va_start(args, fmt);
    vsnprintf(buf + base_len, needed + 1, fmt, args);
    va_end(args);

    return buf;
}

/* Functions to get user information */
/* I just implement windows info while */
char* get_system_info() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
        
    char* str = format_dynamic(
        "Processor Architecture: %u\n"
        "Number of Processors: %u\n"
        "Page Size: %u bytes\n"
        "Processor Type: %u\n"
        "Processor Level: %u\n\n",
        si.wProcessorArchitecture,
        si.dwNumberOfProcessors,
        si.dwPageSize,
        si.dwProcessorType,
        si.wProcessorLevel
    );

    return str;
}

char* get_host_name() {
    char computer_name[256];
    DWORD size = sizeof(computer_name);
    if (GetComputerNameA(computer_name, &size)) {
        char* str = format_dynamic("Host Name: %s\n\n", computer_name);
        return str;
    }
    return format_dynamic("Failed to get host name.\n\n");
}

char* get_user_name() {
    char user_name[256];
    DWORD size = sizeof(user_name);
    if (GetUserNameA(user_name, &size)) {
        char* str = format_dynamic("User: %s\n\n", user_name);
        return str;
    }
    return format_dynamic("Failed to get user name.\n\n");
}

char* get_private_ip() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return format_dynamic("Failed to initialize Winsock.\n\n");
    }

    char host_name[256];
    if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR) {
        WSACleanup();
        return format_dynamic("Failed to get host name.\n\n");
    }

    struct hostent* he = gethostbyname(host_name);
    if (he == NULL) {
        WSACleanup();
        return format_dynamic("Failed to get host information.\n\n");
    }

    struct in_addr ip_addr;
    memcpy(&ip_addr, he->h_addr_list[0], sizeof(struct in_addr));

    char* str = format_dynamic("Private IP: %s\n\n", inet_ntoa(ip_addr));

    WSACleanup();
    return str;
}


/* Pull all those functions together */
char* info_pc() {
    char* info = NULL;

    APPEND_AND_FREE(info, get_system_info());
    APPEND_AND_FREE(info, get_host_name());
    APPEND_AND_FREE(info, get_user_name());
    APPEND_AND_FREE(info, get_private_ip());

    return info; // Caller is responsible for freeing the memory
}

/* Python Wrapper */
static PyObject* infoPc(PyObject* self, PyObject* args) {
    char* info = info_pc();
    if (!info) {
        Py_RETURN_NONE;
    }
    PyObject* result = PyUnicode_FromString(info);
    free(info);  // free dynamically allocated string
    return result;
}

/* Module methods table */
static PyMethodDef PCInfoMethods[] = {
    {"infoPc", infoPc, METH_NOARGS, "Returns system information as a string"},
    {NULL, NULL, 0, NULL}
};

/* Module definition */
static struct PyModuleDef pcinfo_module = {
    PyModuleDef_HEAD_INIT,
    "pcinfo",                             // Module name
    "C module that provides PC information",  // Module docstring
    -1,                                   // Size of per-interpreter state of the module
    PCInfoMethods
};

/* Module initialization function */
PyMODINIT_FUNC PyInit_pcinfo(void) {
    return PyModule_Create(&pcinfo_module);
}
