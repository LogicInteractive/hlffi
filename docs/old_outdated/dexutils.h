#pragma once

#ifndef DEX_UTILS_H
#define DEX_UTILS_H
//#define DEX_UTILS_SKIP_WSTRING

#include "hl.h"
#include <codecvt>
#include <string>
#include <stdarg.h> 
#include <vector>
#include <map>
#include <mutex>

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} IRect;

static hl_type* HL_TSTRING = NULL;

/* Call this with *any* vstring* coming from Haxe – even "" */
static inline void cache_string_type(vstring* s)
{
    if (!HL_TSTRING) HL_TSTRING = s->t;
}


inline  void* hl_to_carr2(varray* hl_array, size_t elem_size, bool is_pointer_type = true)
{
    size_t n = hl_array->size;
    void* c_array = malloc(n * elem_size);

    if (c_array == NULL) {
        return NULL;  // Allocation failed
    }

    if (is_pointer_type) {
        void** hl_data = hl_aptr(hl_array, void*);
        for (size_t i = 0; i < n; ++i) {
            memcpy((char*)c_array + i * elem_size, hl_data[i], elem_size);
        }
    }
    else {
        void* hl_data = hl_aptr(hl_array, void);
        memcpy(c_array, hl_data, n * elem_size);
    }

    return c_array;
}

#define HL_CARR_VAR(_var, _array, _type, _is_pointer) \
    _type* _var = (_type*)hl_to_carr2(_array, sizeof(_type), _is_pointer)


inline std::string wstring_to_utf8(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

inline vstring* cstr_to_hlstr(const char* str)
{
    int len = (int)strlen(str);
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((len + 1) << 1);
    hl_from_utf8(ubuf, len, str);

    vstring* vstr = (vstring*)hl_gc_alloc_raw(sizeof(vstring));
    vstr->bytes = ubuf;
    vstr->length = len;
    vstr->t = &hlt_bytes;
    return vstr;
}

inline vbyte* utf8_to_utf16(vbyte* str, int pos, int* size)
{
    int ulen = hl_utf8_length(str, pos);
    uchar* s = (uchar*)hl_gc_alloc_noptr((ulen + 1) * sizeof(uchar));
    hl_from_utf8(s, ulen, (char*)(str + pos));
    *size = ulen << 1;
    return (vbyte*)s;
}

inline wchar_t* str2wchar(vstring* v)
{
    return (wchar_t*)(v->bytes);
}

inline vstring* utf8_to_hlstr(const char* str) {
    int strLen = (int)strlen(str);
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((strLen + 1) << 1);
    hl_from_utf8(ubuf, strLen, str);

    vstring* vstr = (vstring*)hl_gc_alloc_raw(sizeof(vstring));

    vstr->bytes = ubuf;
    vstr->length = strLen;
    vstr->t = &hlt_bytes;
    return vstr;
}

inline uchar* utf8_to_uchar(const char* str)
{
    int strLen = (int)strlen(str);
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((strLen + 1) << 1);
    hl_from_utf8(ubuf, strLen, str);
    return ubuf;
}

inline uchar* uint8t_to_uchar(uint8_t* strarr, int len)
{
    if (len == 0)
        return nullptr;
    const char* myString = reinterpret_cast<const char*>(strarr);
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((len + 1) << 1);
    hl_from_utf8(ubuf, len, myString);
    return ubuf;
}

inline vstring* utf8_to_hlstr(const std::string& str) {
    return utf8_to_hlstr(str.c_str());
}

inline vstring* wstring_to_hlstr(const std::wstring& wstr)
{
    int strLen = (int)wstr.length();
    vstring* vstr = (vstring*)hl_gc_alloc_raw(sizeof(vstring));
    const wchar_t* wstr_c = wstr.c_str();
    vstr->bytes = (uchar*)wstr_c;
    //memcpy(&vstr->bytes, wstr.c_str(), strLen * sizeof(uchar));
    //vstr->bytes[strLen] = 0; // null-terminate the string
    vstr->length = strLen;
    vstr->t = &hlt_bytes;
    return vstr;
}

#ifndef DEX_UTILS_SKIP_WSTRING

//inline unsigned char* wstring_to_unsigned_char_allocation(const std::wstring& input)
//{
//    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0, NULL, NULL);
//    unsigned char* output = new unsigned char[size_needed + 1]; // +1 for the null terminator
//    WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), reinterpret_cast<char*>(output), size_needed, NULL, NULL);
//    output[size_needed] = '\0';
//    return output;
//}

inline unsigned char* wstring_to_unsigned_char_allocation(const std::wstring& input)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string utf8 = converter.to_bytes(input);

    unsigned char* output = new unsigned char[utf8.size() + 1];
    std::memcpy(output, utf8.data(), utf8.size());
    output[utf8.size()] = '\0';
    return output;
}

inline wchar_t* wstring_to_wchar_allocation(const std::wstring& input)
{
    wchar_t* output = new wchar_t[input.size() + 1];
    std::wcscpy(output, input.c_str());
    return output;
}

#endif // DEX_UTILS_SKIP_WSTRING

#define C_TO_HLSTR(cstr) utf8_to_hlstr(cstr)
#define HLSTR_TO_C(vstr) (vstr ? (const char*)hl_to_utf8(vstr->bytes) : NULL)



static void handle_call_exception(const char* where, vdynamic* exc)
{
    printf("Uncaught exception : % s : % s\n", where, hl_to_utf8(hl_to_string(exc)));
}

static vdynamic* call_haxe_func(vclosure* closure, bool safe = true, bool hlGlobalInit = true, bool hlRegisterThread = true, int numArgs = 0, ...)
{
    vdynamic* exc = NULL;
    if (!closure)
        return exc;

    bool isExc = false;

    if (hlGlobalInit)
        hl_global_init();

    if (hlRegisterThread)
        hl_register_thread(0);

    if (numArgs == 0)
    {
        if (safe)
            exc = hl_dyn_call_safe(closure, NULL, 0, &isExc);
        else
            exc = hl_dyn_call(closure, NULL, 0);
    }
    else
    {
        va_list args;
        va_start(args, numArgs);

        vdynamic** vargs = (vdynamic**)malloc(sizeof(vdynamic*) * numArgs);
        for (int i = 0; i < numArgs; ++i) {
            vargs[i] = (vdynamic*)malloc(sizeof(vdynamic));
            hl_type* type = va_arg(args, hl_type*);
            vargs[i]->t = type;

            if (type == &hlt_bool) {
                vargs[i]->v.b = (bool)va_arg(args, bool);
            }
            else if (type == &hlt_i32) {
                vargs[i]->v.i = va_arg(args, int);
            }
            else if (type == &hlt_i64) {
                vargs[i]->v.i64 = va_arg(args, long long);
            }
            else if (type == &hlt_f32) {
                vargs[i]->v.f = (float)va_arg(args, float);
            }
            else if (type == &hlt_f64) {
                vargs[i]->v.d = va_arg(args, double);
            }
            else if (type == &hlt_bytes) {
                vbyte* vb = va_arg(args, vbyte*);
                vargs[i]->v.ptr = vb;
            }
            //else if (type == &hlt_dynobj) {
            //	void* vb = va_arg(args, void*);
            //	vargs[i]->v.ptr;
            //}
            // ... add other types as needed
        }

        va_end(args);
        if (safe)
            exc = hl_dyn_call_safe(closure, vargs, numArgs, &isExc);
        else
            exc = hl_dyn_call(closure, vargs, numArgs);

        for (int i = 0; i < numArgs; ++i)
            free(vargs[i]);
        free(vargs);

    }

    if (hlGlobalInit)
        hl_global_free();

    if (hlRegisterThread)
        hl_unregister_thread();

    if (isExc)
    {
        handle_call_exception("exception", exc);
        return NULL;
    }
    else
        return exc;
}

/*
    rootCallback(callback);
    unrootCallback(callback);
*/

static std::vector<vclosure**> callbackVector;
static std::mutex callbackMutex;

static void rootCallback(vclosure* callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex);
    vclosure** stablePtr = new vclosure * (callback);
    callbackVector.push_back(stablePtr);
    hl_add_root(stablePtr);
}

static void unrootCallback(vclosure* callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex);
    auto it = std::find_if(callbackVector.begin(), callbackVector.end(), [callback](vclosure** ptr) { return *ptr == callback; });
    if (it != callbackVector.end())
    {
        hl_remove_root(*it);
        delete* it;
        callbackVector.erase(it);
    }
}


static double pack_f32_f64(float a, float b)
{
    return (double)a + (double)b / 65536.0;
}

static void unpack_f64_f32(double packed, float* a, float* b)
{
    *a = (float)((int)packed); // Truncates fractional part
    *b = (float)((packed - (double)(int)packed) * 65536.0);
}




// In dexutils.h
#define DECLARE_CALLBACK_MAP() \
    static std::map<std::pair<void*, vclosure*>, vclosure*> g_callbacks; \
    static void closure_store(void* owner, vclosure* cb) { \
        g_callbacks[std::make_pair(owner, cb)] = cb; \
        hl_add_root(&g_callbacks[std::make_pair(owner, cb)]); \
    } \
    static vclosure* closure_get(void* owner, vclosure* cb) { \
        auto it = g_callbacks.find(std::make_pair(owner, cb)); \
        return (it != g_callbacks.end()) ? it->second : nullptr; \
    } \
    static void closure_remove(void* owner, vclosure* cb) { \
        auto it = g_callbacks.find(std::make_pair(owner, cb)); \
        if (it != g_callbacks.end()) { \
            hl_remove_root(&it->second); \
            g_callbacks.erase(it); \
        } \
    }

// Usage example:
DECLARE_CALLBACK_MAP()

/* ---------- 1.  enum for argument kinds ------------------------- */
typedef enum {
    HLT_BOOL,
    HLT_I32,
    HLT_I64,
    HLT_F32,
    HLT_F64,
    HLT_BYTES,      /* vbyte*                            → Bytes */
    HLT_STRING,     /* const char* UTF‑8                 → String */
    HLT_ABSTRACT,    /* (void*, const char* absName)      → Abstract */
    HLT_OBJECT,
    HLT_UNKNOWN
} HLType;


inline const char* hl_type_name(hl_type* t) {
    if (!t) return "(null)";
    switch ((hl_type_kind)t->kind) {
    case HVOID:     return "Void";
    case HI32:      return "Int32";
    case HI64:      return "Int64";
    case HF32:      return "Float32";
    case HF64:      return "Float64";
    case HBOOL:     return "Bool";
    case HBYTES:    return "Bytes";
    case HDYN:      return "Dynamic";
    case HFUN:      return "Function";
    case HARRAY:    return "Array";
    case HTYPE:     return "Type";
    case HREF:      return "Ref<T>";
    case HVIRTUAL:  return "Virtual";
    case HDYNOBJ:   return "DynamicObject";
    case HABSTRACT: return "Abstract";
    case HENUM:     return "Enum";
    case HNULL:     return "Null<T>";
    case HMETHOD:   return "Method";
    case HSTRUCT:   return "Struct";
    case HPACKED:   return "Packed";
    case HOBJ:
    {
        if (t == HL_TSTRING)
            return "String";

        return "Object";
    }
    default:        return "(unknown)";
    }
}

static void dump_closure_signature(vclosure* cb)
{
    if (!cb || !cb->t || cb->t->kind != HFUN) {
        printf("Not a valid function closure.\n");
        return;
    }

    hl_type_fun* tfun = (hl_type_fun*)cb->t->fun;
    int nargs = tfun->nargs;
    const char* ret = hl_type_name(tfun->ret);
    printf("Function takes %d arguments, returns: %s\n", nargs, ret);

    for (int i = 0; i < tfun->nargs; ++i) {
        hl_type* t = tfun->args[i];
        const char* arg = hl_type_name(t);
        printf("  Arg %d: %s\n", i, arg);
    }
}

static HLType map_hltype_enum(hl_type* t)
{
    if (!t) return HLT_OBJECT;

    switch ((hl_type_kind)t->kind)
    {
    case HBOOL:     return HLT_BOOL;
    case HI32:      return HLT_I32;
    case HI64:      return HLT_I64;
    case HF32:      return HLT_F32;
    case HF64:      return HLT_F64;
    case HBYTES:    return HLT_BYTES;
    case HOBJ:      return (t == HL_TSTRING) ? HLT_STRING : HLT_OBJECT;
    case HABSTRACT: return HLT_ABSTRACT;
    default:        return HLT_OBJECT;
    }
}

static void get_hl_arg_types(vclosure* cb, std::vector<HLType>* out)
{
    if (!cb || !cb->t || cb->t->kind != HFUN)
        return;

    hl_type_fun* tfun = (hl_type_fun*)cb->t;
    for (int i = 0; i < tfun->nargs; ++i) {
        out->push_back(map_hltype_enum(tfun->args[i]));
    }
}

// expression macro, returns a C pointer to the element buffer
#define HL_NEW_ARR(DESC, CTYPE, NAME, N)      \
  varray *NAME##_arr = hl_alloc_array(&(DESC), (N)); \
  CTYPE  *NAME       = hl_aptr(NAME##_arr, CTYPE);

#define HL_NEW_DYN_ARR(NAME,N)                           \
  varray *NAME##_arr = hl_alloc_array(&hlt_dyn, (N));    \
  vdynamic **NAME   = hl_aptr(NAME##_arr, vdynamic*)

//#define HL_NEW_DYN_ARR(N) HL_NEW_ARR(hlt_dyn, vdynamic*,(N))
#define HL_I32_ARR(N)   HL_ARR(hlt_i32,     int,        (N))
#define HL_I64_ARR(N)   HL_ARR(hlt_i64,     long long,  (N))
#define HL_F32_ARR(N)   HL_ARR(hlt_f32,     float,      (N))
#define HL_F64_ARR(N)   HL_ARR(hlt_f64,     double,     (N))
#define HL_BOOL_ARR(N)  HL_ARR(hlt_bool,    bool,       (N))
#define HL_BYTES_ARR(N) HL_ARR(hlt_bytes,   vbyte*,     (N))

#define HL_DYN_GET(DYN, FIELD) ((DYN)->v.FIELD)

// Check if dynamic value has a specific type
#define HL_IS_TYPE(DYN, TYPEPTR) ((DYN)->t == &(TYPEPTR))

// Common type checks
#define HL_IS_I32(DYN)    HL_IS_TYPE(DYN, hlt_i32)
#define HL_IS_I64(DYN)    HL_IS_TYPE(DYN, hlt_i64)
#define HL_IS_F32(DYN)    HL_IS_TYPE(DYN, hlt_f32)
#define HL_IS_F64(DYN)    HL_IS_TYPE(DYN, hlt_f64)
#define HL_IS_BOOL(DYN)   HL_IS_TYPE(DYN, hlt_bool)
#define HL_IS_STRING(DYN) HL_IS_TYPE(DYN, hlt_string)
#define HL_IS_BYTES(DYN)  HL_IS_TYPE(DYN, hlt_bytes)
#define HL_IS_ABSTRACT(DYN) HL_IS_TYPE(DYN, hlt_abstract)

/* -----------------------------------------------------------------
   HashLink helper – call a Haxe vclosure with typed var‑args
   Features
     • safe/unsafe call (flag)
     • optional hl_global_init / hl_register_thread guards (flags)
     • typed arguments chosen with an enum
     • UTF‑8 → String handled internally
     • raw pointers mapped to hl.Abstract<"...*"> via name‑based cache
   ----------------------------------------------------------------- */

   /* ---------- 2.  UTF‑8 → HL string (cached type ptr elsewhere) --- */
extern vstring* utf8_to_hlstr(const char* utf8);

/* ---------- 3.  cache for each abstract name -------------------- */
typedef struct AbsCache {
    const char* name;   /* literal pointer from call site      */
    hl_type* t;      /* allocated HL abstract type          */
    struct AbsCache* next;
} AbsCache;

static AbsCache* abs_cache = NULL;

static hl_type* get_abs_type(const char* name) {
    for (AbsCache* c = abs_cache; c; c = c->next)
        if (!strcmp(c->name, name)) return c->t;

    hl_type* nt = nullptr;// hl_alloc_type(HL_TYPE_ABSTRACT);
    //nt->obj_name = (char*)name;                        /* debugger aid */

    AbsCache* node = (AbsCache*)malloc(sizeof(AbsCache));
    node->name = name;
    node->t = nt;
    node->next = abs_cache;
    abs_cache = node;
    return nt;
}

/* ---------- 4.  build argv[] on the GC heap --------------------- */


/*
static vdynamic** make_argv(int num, va_list ap) {
    if (num == 0) return NULL;
    vdynamic** argv = hl_alloc_array(&hlt_dyn, num);

    for (int i = 0; i < num; ++i) {
        ArgKind k = (ArgKind)va_arg(ap, int);

        switch (k) {
        case ARG_BOOL:   argv[i] = hl_make_dyn((void*)(intptr_t)va_arg(ap, int), &hlt_bool); break;
        case ARG_I32:    argv[i] = hl_make_dyn((void*)(intptr_t)va_arg(ap, int), &hlt_i32);  break;

        case ARG_I64: {
            vdynamic* d = hl_alloc_dynamic(&hlt_i64);
            d->v.i64 = va_arg(ap, long long);
            argv[i] = d;
        } break;

        case ARG_F32: {
            vdynamic* d = hl_alloc_dynamic(&hlt_f32);
            d->v.f = (float)va_arg(ap, double);
            argv[i] = d;
        } break;

        case ARG_F64: {
            vdynamic* d = hl_alloc_dynamic(&hlt_f64);
            d->v.d = va_arg(ap, double);
            argv[i] = d;
        } break;

        case ARG_BYTES:
            argv[i] = hl_make_dyn(va_arg(ap, void*), &hlt_bytes);
            break;

        case ARG_STRING:
            argv[i] = (vdynamic*)utf8_to_hlstr(va_arg(ap, const char*));
            break;

        case ARG_ABSTRACT: {
            void* ptr = va_arg(ap, void*);
            const char* name = va_arg(ap, const char*);
            argv[i] = hl_make_dyn(ptr, get_abs_type(name));
        } break;
        }
    }
    return argv;
}
*/

static vdynamic** make_argv(int num, va_list ap)
{
    if (num == 0) return NULL;

    HL_NEW_DYN_ARR(argv, num);

    for (int i = 0; i < num; ++i)
    {
        HLType k = (HLType)va_arg(ap, int);
        switch (k)
        {
        case HLT_BOOL: {
            vdynamic* d = hl_alloc_dynamic(&hlt_bool);
            d->v.b = (bool)va_arg(ap, int);
            argv[i] = d;
        } break;

        case HLT_I32: {
            vdynamic* d = hl_alloc_dynamic(&hlt_i32);
            d->v.i = va_arg(ap, int);
            argv[i] = d;
        } break;

        case HLT_I64: {
            vdynamic* d = hl_alloc_dynamic(&hlt_i64);
            d->v.i64 = va_arg(ap, long long);
            argv[i] = d;
        } break;

        case HLT_F32: {
            vdynamic* d = hl_alloc_dynamic(&hlt_f32);
            d->v.f = (float)va_arg(ap, double);
            argv[i] = d;
        } break;

        case HLT_F64: {
            vdynamic* d = hl_alloc_dynamic(&hlt_f64);
            d->v.d = va_arg(ap, double);
            argv[i] = d;
        } break;

        case HLT_BYTES:
            argv[i] = hl_make_dyn(va_arg(ap, void*), &hlt_bytes);
            break;

        case HLT_STRING:
            argv[i] = (vdynamic*)utf8_to_hlstr(va_arg(ap, const char*));
            break;

        case HLT_ABSTRACT:
            argv[i] = hl_make_dyn(va_arg(ap, void*), &hlt_abstract);
            break;
        }
    }

    return argv;
}

/* ---------- 5.  flag bits to enable optional guards ------------- */
#define CALL_HL_SAFE       1
#define CALL_HL_GLOBAL     2
#define CALL_HL_REGISTER   4
#define CALL_HL_ALL  (CALL_HL_SAFE|CALL_HL_GLOBAL|CALL_HL_REGISTER)

inline int call_haxe_flags(bool handleException = true, bool fromExternalThread = false, bool isGlobalHLFirstCall = false)
{
    int flags = 0;

    /**
    Uses hl_dyn_call_safe instead of hl_dyn_call.
        • Catches any Haxe exception into a vdynamic* rather than aborting the VM.
        • If an exception occurs, your code sees trapped = true and can call handle_call_exception.
        */
    if (handleException)
        flags |= CALL_HL_SAFE;

    /**
      * Wraps the call with hl_global_init() / hl_global_free().
      • Ensures the HashLink runtime is initialized on the first call.
      • You only need this once per process—after that the calls are no‑ops.
    */
    if (isGlobalHLFirstCall)
        flags |= CALL_HL_GLOBAL;

    /**
    * Wraps the call with hl_register_thread(0) / hl_unregister_thread().
    • Registers the current OS thread with the HL VM, so GC roots and exceptions work properly.
    • Only needed if you invoke Haxe from a worker thread; the main thread is already registered.
    */
    if (fromExternalThread)
        flags |= CALL_HL_REGISTER;

    return flags;
}

// --- HLTypeMap<T> ---
template<typename T> struct HLTypeMap;
template<> struct HLTypeMap<int> { static constexpr HLType value = HLT_I32; };
template<> struct HLTypeMap<bool> { static constexpr HLType value = HLT_BOOL; };
template<> struct HLTypeMap<float> { static constexpr HLType value = HLT_F32; };
template<> struct HLTypeMap<double> { static constexpr HLType value = HLT_F64; };
template<> struct HLTypeMap<const char*> { static constexpr HLType value = HLT_STRING; };

// --- to_vdynamic ---
inline vdynamic* to_vdynamic(int i) { auto* d = hl_alloc_dynamic(&hlt_i32); d->v.i = i; return d; }
inline vdynamic* to_vdynamic(bool b) { auto* d = hl_alloc_dynamic(&hlt_bool); d->v.b = b; return d; }
inline vdynamic* to_vdynamic(float f) { auto* d = hl_alloc_dynamic(&hlt_f32); d->v.f = f; return d; }
inline vdynamic* to_vdynamic(double dval) { auto* d = hl_alloc_dynamic(&hlt_f64); d->v.d = dval; return d; }
inline vdynamic* to_vdynamic(const char* s) { return (vdynamic*)utf8_to_hlstr(s); }

// --- from_vdynamic ---
template<typename T> T from_vdynamic(vdynamic* v);
template<> inline int from_vdynamic<int>(vdynamic* v) { return v ? v->v.i : 0; }
template<> inline double from_vdynamic<double>(vdynamic* v) { return v ? v->v.d : 0.0; }
template<> inline bool from_vdynamic<bool>(vdynamic* v) { return v ? v->v.b : false; }
template<> inline const char* from_vdynamic<const char*>(vdynamic* v) {
    return v ? hl_to_utf8(((vstring*)v)->bytes) : nullptr;
}
template<>
inline vdynamic* from_vdynamic<vdynamic*>(vdynamic* v) {
    return v;
}

template<typename Ret = vdynamic*, typename... Args>
Ret hlcall_internal(vclosure* cb, int flags, bool check, Args&&... args) {
    if (!cb || !cb->t || cb->t->kind != HFUN) {
        printf("hlcall: invalid closure\n");
        fflush(stdout);
        return Ret();
    }

    if (!cb) { printf("cb is NULL!\n"); }
    if (!cb->fun) { printf("cb->fun is NULL!\n"); }
    //printf("cb->t->kind = %d\n", cb->t->kind);

    hl_type_fun* tfun = (hl_type_fun*)cb->t->fun;
    constexpr int nargs = sizeof...(Args);
    if (check && tfun->nargs != nargs) {
        printf("hlcall: argument count mismatch. expected %d, got %d\n", tfun->nargs, nargs);
        fflush(stdout);
        return Ret();
    }

    HL_NEW_ARR(hlt_dyn, vdynamic*, argv, nargs);
    bool mismatch = false;
    int i = 0;

    (void)std::initializer_list<int>{
        ([
            &argv, &i, &tfun, &check, &mismatch
        ](auto&& val) {
                using Arg = std::decay_t<decltype(val)>;
                constexpr HLType expected = HLTypeMap<Arg>::value;

                if (check) {
                    HLType actual = map_hltype_enum(tfun->args[i]);
                    if (expected != actual) {
                        printf("hlcall: type mismatch at arg %d: expected %d, got %d\n", i, actual, expected);
                        fflush(stdout);
                        mismatch = true;
                    }
                }

                argv[i++] = to_vdynamic(std::forward<decltype(val)>(val));
            }(args), 0)...
    };

    if (mismatch)
        return Ret();

    bool safe = flags & CALL_HL_SAFE;
    bool doGlobal = flags & CALL_HL_GLOBAL;
    bool doReg = flags & CALL_HL_REGISTER;

    if (doGlobal)   hl_global_init();
    if (doReg)      hl_register_thread(0);

    bool hadException = false;
    vdynamic* result = safe
        ? hl_dyn_call_safe(cb, argv, nargs, &hadException)
        : hl_dyn_call(cb, argv, nargs);

    if (doReg)      hl_unregister_thread();
    if (doGlobal)   hl_global_free();

    //if (!result) {
    //    printf("Callback returned NULL\n");
    //}
    //else {
    //    printf("Got type kind: %d\n", result->t->kind);
    //}
    //fflush(stdout);
    if (safe && hadException) {
        handle_call_exception("Haxe exception", result);
        return Ret();  // return default if exception occurred
    }
    fflush(stdout);
    return from_vdynamic<Ret>(result);
}

template<typename Ret = vdynamic*, typename... Args>
inline Ret call_haxe(vclosure* cb, Args&&... args) {
    return hlcall_internal<Ret>(cb, CALL_HL_SAFE, true, std::forward<Args>(args)...);
}

static vdynamic* call_haxe_2(vclosure* cl, int flags, int argc, ...) {
    if (!cl) return NULL;

    if (flags & CALL_HL_GLOBAL)   hl_global_init();
    if (flags & CALL_HL_REGISTER) hl_register_thread(0);

    va_list ap;  va_start(ap, argc);
    vdynamic** argv = make_argv(argc, ap);
    va_end(ap);

    bool trapped = false;
    vdynamic* ret = (flags & CALL_HL_SAFE)
        ? hl_dyn_call_safe(cl, argv, argc, &trapped)
        : hl_dyn_call(cl, argv, argc);

    if (flags & CALL_HL_GLOBAL)   hl_global_free();
    if (flags & CALL_HL_REGISTER) hl_unregister_thread();

    if (trapped) {
        /* user‑defined handler; implement as you like */
        handle_call_exception("exception", ret);
        return NULL;
    }
    return ret;
}

// 0 args
template<typename Ret = vdynamic*>
Ret call_haxe_3(vclosure* cb) {
    return hl_call0(Ret, cb);
}

// 1 arg
template<typename Ret = vdynamic*, typename A1>
Ret call_haxe_3(vclosure* cb, A1 a1) {
    return hl_call1(Ret, cb, A1, a1);
}

// 2 args
template<typename Ret = vdynamic*, typename A1, typename A2>
Ret call_haxe_3(vclosure* cb, A1 a1, A2 a2) {
    return hl_call2(Ret, cb, A1, a1, A2, a2);
}

// 3 args
template<typename Ret = vdynamic*, typename A1, typename A2, typename A3>
Ret call_haxe_3(vclosure* cb, A1 a1, A2 a2, A3 a3) {
    return hl_call3(Ret, cb, A1, a1, A2, a2, A3, a3);
}

// 4 args
template<typename Ret = vdynamic*, typename A1, typename A2, typename A3, typename A4>
Ret call_haxe_3(vclosure* cb, A1 a1, A2 a2, A3 a3, A4 a4) {
    return hl_call4(Ret, cb, A1, a1, A2, a2, A3, a3, A4, a4);
}

#define HL_CALL_3(Ret, cb, ...) call_haxe_fast<Ret>(cb, __VA_ARGS__)
//auto r = HL_CALL_3(int, cb, 123, 456);



#endif // DEX_UTILS_H