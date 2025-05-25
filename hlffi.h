/*
   hlffi.h — HashLink FFI one‑header bridge (v2.2‑beta)
   =====================================================
   A single‑file library that embeds the HashLink VM (Haxe) into any C/C++
   application or plugin — Unreal, Godot, custom engines, CLI tools, you name it.

   ┌──────────────────────────── HOW TO USE ────────────────────────────┐
   │   //  In ONE translation unit only                                  │
   │   #define HLFFI_IMPLEMENTATION                                      │
   │   #include "hlffi.h"                                               │
   │                                                                    │
   │   //  (Optional) Tweak feature flags *before* the include           │
   │   #define HLFFI_EXT_UNREAL        // Enable UE helper macros        │
   │   // Everything else is ON by default (see §1).                     │
   └────────────────────────────────────────────────────────────────────┘

   Build :  MSVC ➜ link against hl.lib            (HashLink official)
            GCC/Clang ➜ -lhl                      

   License: MIT — do whatever you want. Credits appreciated.
*/
#ifndef HLFFI_H
#define HLFFI_H

/* ============================= 0.  INCLUDES ============================= */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1.  FEATURE FLAGS
 * -------------------------------------------------------------------------
 * Everything is ENABLED by default. Flip a flag to 0 before the #include
 * if you need a micro‑footprint build.                                     */
#ifndef HLFFI_EXT_STATIC
#  define HLFFI_EXT_STATIC   1   /* Player.spawn(), etc.            */
#endif
#ifndef HLFFI_EXT_VAR
#  define HLFFI_EXT_VAR      1   /* Game.score access               */
#endif
#ifndef HLFFI_EXT_VALUE
#  define HLFFI_EXT_VALUE    1   /* Boxed "variant" type            */
#endif
#ifndef HLFFI_EXT_ERRORS
#  define HLFFI_EXT_ERRORS   1   /* hlffi_result enum + msg buffer  */
#endif
#ifndef HLFFI_EXT_UNREAL
#  define HLFFI_EXT_UNREAL   0   /* UE_LOG wrappers                 */
#endif

/* =========================================================================
 * 2.  FORWARD DECLS & ENUMS                                                */
typedef struct hlffi_vm hlffi_vm;       /* opaque */

#if HLFFI_EXT_ERRORS
typedef enum {
    HLFFI_OK         =  0,
    HLFFI_E_INIT     = -1,
    HLFFI_E_LOAD     = -2,
    HLFFI_E_NOTFOUND = -3,
    HLFFI_E_TYPE     = -4,
    HLFFI_E_CALL     = -5,
    HLFFI_E_MEMORY   = -6,
    HLFFI_E_PARAM    = -7
} hlffi_result;
#endif

#if HLFFI_EXT_VALUE
typedef enum {
    HLFFI_T_VOID, HLFFI_T_BOOL, HLFFI_T_INT, HLFFI_T_FLOAT,
    HLFFI_T_DOUBLE, HLFFI_T_STRING, HLFFI_T_OBJECT, HLFFI_T_NULL
} hlffi_val_kind;
typedef struct hlffi_value hlffi_value;
#endif

/* =========================================================================
 * 3.  LIFECYCLE & STATE                                                    */
hlffi_vm*    hlffi_create          (void);

#if HLFFI_EXT_ERRORS
hlffi_result hlffi_init            (hlffi_vm*, const char* main_hl, int argc, char** argv);
hlffi_result hlffi_load_file       (hlffi_vm*, const char* path);
hlffi_result hlffi_load_mem        (hlffi_vm*, const void* data, int size);
hlffi_result hlffi_call_entry      (hlffi_vm*);
const char*  hlffi_error           (hlffi_vm*);
#else /* simple bool adaptors (always available as inline) */
bool         hlffi_init_bool       (hlffi_vm*, const char* main_hl, int argc, char** argv);
bool         hlffi_load_file_bool  (hlffi_vm*, const char* path, const char** errmsg);
bool         hlffi_load_mem_bool   (hlffi_vm*, const void* data, int size, const char** errmsg);
bool         hlffi_call_entry_bool (hlffi_vm*);
#endif

void         hlffi_close           (hlffi_vm*);
void         hlffi_destroy         (hlffi_vm*);
bool         hlffi_ready           (hlffi_vm*);

/* =========================================================================
 * 4.  LOW‑LEVEL LOOKUP + CALLS                                             */
void*        hlffi_lookup          (hlffi_vm*, const char* name, int nargs);
int32_t      hlffi_call0           (void* f);
int32_t      hlffi_call1           (void* f, void* a0);
int32_t      hlffi_call2           (void* f, void* a0, void* a1);
int32_t      hlffi_call3           (void* f, void* a0, void* a0b, void* a1);
int32_t      hlffi_call4           (void* f, void* a0, void* a1, void* a2, void* a3);
/*  A slower but super‑generic wrapper if you need >4 args at runtime  */
int32_t      hlffi_call_variadic   (void* f, int argc, /* void** */ ...);

/* =========================================================================
 * 5.  STATIC METHOD HELPERS (if ON)                                        */
#if HLFFI_EXT_STATIC
bool         hlffi_call_static            (hlffi_vm*, const char* cls, const char* fun);
bool         hlffi_call_static_int        (hlffi_vm*, const char* cls, const char* fun, int);
bool         hlffi_call_static_float      (hlffi_vm*, const char* cls, const char* fun, double);
bool         hlffi_call_static_string     (hlffi_vm*, const char* cls, const char* fun, const char*);
int          hlffi_call_static_ret_int    (hlffi_vm*, const char* cls, const char* fun);
double       hlffi_call_static_ret_float  (hlffi_vm*, const char* cls, const char* fun);
char*        hlffi_call_static_ret_string (hlffi_vm*, const char* cls, const char* fun); /* malloc → free */
#endif

/* =========================================================================
 * 6.  STATIC VARIABLE ACCESS (if ON)                                       */
#if HLFFI_EXT_VAR
int          hlffi_get_static_int        (hlffi_vm*, const char* cls, const char* var);
double       hlffi_get_static_float      (hlffi_vm*, const char* cls, const char* var);
char*        hlffi_get_static_string     (hlffi_vm*, const char* cls, const char* var);
bool         hlffi_set_static_int        (hlffi_vm*, const char* cls, const char* var, int);
bool         hlffi_set_static_float      (hlffi_vm*, const char* cls, const char* var, double);
bool         hlffi_set_static_string     (hlffi_vm*, const char* cls, const char* var, const char*);
#endif

/* =========================================================================
 * 7.  BOXED VALUE LAYER (if ON)                                            */
#if HLFFI_EXT_VALUE
hlffi_value*   hlffi_val_int          (int);
hlffi_value*   hlffi_val_float        (double);
hlffi_value*   hlffi_val_bool         (bool);
hlffi_value*   hlffi_val_string       (const char* utf8);
hlffi_value*   hlffi_val_null         (void);
void           hlffi_val_free         (hlffi_value*);
hlffi_val_kind hlffi_val_type         (hlffi_value*);
int            hlffi_val_as_int       (hlffi_value*);
double         hlffi_val_as_float     (hlffi_value*);
bool           hlffi_val_as_bool      (hlffi_value*);
char*          hlffi_val_as_string    (hlffi_value*); /* malloc → free */
#endif

/* =========================================================================
 * 8.  THREADING & GC                                                       */
void         hlffi_thread_attach    (void);
void         hlffi_thread_detach    (void);
void         hlffi_gc_stats         (uint32_t* objects, uint64_t* heap_bytes);
void         hlffi_gc_collect       (void);

/* =========================================================================
 * 9.  VERSION                                                              */
const char*  hlffi_version          (void);  /* e.g. "2.2-beta" */

/* =========================================================================
 * 10.  UNREAL ENGINE WRAPPERS (optional)                                   */
#if HLFFI_EXT_UNREAL && defined(UNREAL_ENGINE_H)
#  define HLFFI_UE_CALL(vm, cls, fn)                                           \
       do { if(!hlffi_call_static(vm, cls, fn))                                \
               UE_LOG(LogTemp, Error, TEXT("HLFFI call failed: %s::%s"),      \
                      TEXT(cls), TEXT(fn));                                    \
       } while(0)
#  define HLFFI_UE_CALL_FLOAT(vm, cls, fn, val)                                \
       do { if(!hlffi_call_static_float(vm, cls, fn, val))                     \
               UE_LOG(LogTemp, Error, TEXT("HLFFI call failed: %s::%s(%f)"),  \
                      TEXT(cls), TEXT(fn), val);                               \
       } while(0)
#endif /* UE */

/* =========================================================================
 * 11.  IMPLEMENTATION SECTION                                              */
#ifdef HLFFI_IMPLEMENTATION

/* ---- 11.1  Headers ----------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*  HashLink public headers  */
extern "C" {
#  include <hl.h>
#  include <hlmodule.h>
}

/* ---- 11.2  Internal bookkeeping --------------------------------------- */
#define HLFFI_VER_STR "2.2-beta"
#define HLFFI_ERRBUF  256

struct hlffi_vm {
    hl_module* mod;
    hl_code*   code;
    vclosure*  entry;
    bool       ready;
    bool       init;
#if HLFFI_EXT_ERRORS
    int        err_code;
    char       err_msg[HLFFI_ERRBUF];
#endif
};

#if HLFFI_EXT_ERRORS
#  define ERR(vm, code, fmt, ...)                                             \
       do { if(vm) { (vm)->err_code = (code);                                 \
                     snprintf((vm)->err_msg, HLFFI_ERRBUF, fmt, __VA_ARGS__); }\
       return (code); } while(0)
#else
#  define ERR_BOOL(ret, msgptr, literal) do { if(msgptr) *(msgptr) = literal; return (ret); } while(0)
#endif

/* ---- 11.3  Lifecycle ---------------------------------------------------- */
hlffi_vm* hlffi_create(void){
    hlffi_vm* vm = (hlffi_vm*)calloc(1, sizeof(hlffi_vm));
#if HLFFI_EXT_ERRORS
    vm->err_code = HLFFI_OK;
    vm->err_msg[0] = '\0';
#endif
    return vm;
}

#if HLFFI_EXT_ERRORS
static inline void clear_err(hlffi_vm* vm){ if(vm){ vm->err_code = HLFFI_OK; vm->err_msg[0]='\0'; } }
hlffi_result hlffi_init(hlffi_vm* vm, const char* main_hl, int argc, char** argv){
    if(!vm) return HLFFI_E_PARAM;
    if(vm->init) return HLFFI_OK;
    hl_global_init();
    hl_sys_init((void**)argv, argc, (char*)main_hl);
    hl_register_thread(NULL);
    vm->init = true;
    return HLFFI_OK;
}
#else
bool hlffi_init_bool(hlffi_vm* vm, const char* main_hl, int argc, char** argv){
    if(!vm || vm->init) return false;
    hl_global_init();
    hl_sys_init((void**)argv, argc, (char*)main_hl);
    hl_register_thread(NULL);
    vm->init = true;
    return true;
}
#endif

/* --- shared loader helper ---------------------------------------------- */
static bool loader_common(hlffi_vm* vm, const unsigned char* bytes, int size, const char** errmsg){
    char* hlmsg = NULL;
    vm->code = hl_code_read(bytes, size, &hlmsg);
    if(!vm->code){
#if HLFFI_EXT_ERRORS
        ERR(vm, HLFFI_E_LOAD, "%s", hlmsg ? hlmsg : "hl_code_read failed");
#else
        ERR_BOOL(false, errmsg, "hl_code_read failed");
#endif
    }
    vm->mod = hl_module_alloc(vm->code);
    if(!vm->mod){
#if HLFFI_EXT_ERRORS
        ERR(vm, HLFFI_E_LOAD, "%s", "hl_module_alloc failed");
#else
        ERR_BOOL(false, errmsg, "hl_module_alloc failed");
#endif
    }
    hl_module_init(vm->mod, false, false);
    int ep = vm->code->entrypoint;
    vm->entry = (vclosure*)malloc(sizeof(vclosure));
    vm->entry->t   = vm->code->functions[vm->mod->functions_indexes[ep]].type;
    vm->entry->fun = vm->mod->functions_ptrs[ep];
    vm->entry->hasValue = false;
    vm->ready = true;
    return true;
}

#if HLFFI_EXT_ERRORS
hlffi_result hlffi_load_file(hlffi_vm* vm, const char* path){
    if(!vm || !vm->init) return HLFFI_E_INIT;
    FILE* fp = fopen(path, "rb");
    if(!fp) ERR(vm, HLFFI_E_LOAD, "%s", "open file failed");
    fseek(fp, 0, SEEK_END); long len = ftell(fp); fseek(fp, 0, SEEK_SET);
    unsigned char* buf = (unsigned char*)malloc(len);
    if(fread(buf,1,len,fp)!=(size_t)len){ fclose(fp); free(buf); ERR(vm, HLFFI_E_LOAD, "%s", "read file failed"); }
    fclose(fp);
    bool ok = loader_common(vm, buf, (int)len, NULL); free(buf);
    return ok ? HLFFI_OK : vm->err_code;
}

hlffi_result hlffi_load_mem(hlffi_vm* vm, const void* data, int size){
    if(!vm || !vm->init) return HLFFI_E_INIT;
    bool ok = loader_common(vm, (const unsigned char*)data, size, NULL);
    return ok ? HLFFI_OK : vm->err_code;
}

hlffi_result hlffi_call_entry(hlffi_vm* vm){
    if(!vm || !vm->ready) return HLFFI_E_INIT;
    hl_call0(vm->entry);
    return HLFFI_OK;
}

const char* hlffi_error(hlffi_vm* vm){ return vm ? vm->err_msg : ""; }

#else /* bool flavours ---------------------------------------------------- */
bool hlffi_load_file_bool(hlffi_vm* vm, const char* path, const char** err){
    if(!vm || !vm->init) return false;
    FILE* fp = fopen(path, "rb");
    if(!fp) ERR_BOOL(false, err, "open file failed");
    fseek(fp,0,SEEK_END); long len = ftell(fp); fseek(fp,0,SEEK_SET);
    unsigned char* buf = (unsigned char*)malloc(len);
    if(fread(buf,1,len,fp)!=(size_t)len){ fclose(fp); free(buf); ERR_BOOL(false, err, "read file failed"); }
    fclose(fp);
    bool ok = loader_common(vm, buf, (int)len, err); free(buf);
    return ok;
}

bool hlffi_load_mem_bool(hlffi_vm* vm, const void* data, int size, const char** err){
    return loader_common(vm, (const unsigned char*)data, size, err);
}

bool hlffi_call_entry_bool(hlffi_vm* vm){ if(!vm || !vm->ready) return false; hl_call0(vm->entry); return true; }
#endif /* errors / bool */

void hlffi_close(hlffi_vm* vm){ if(!vm || !vm->init) return; hl_unregister_thread(); hl_global_free(); vm->init = false; vm->ready = false; }
void hlffi_destroy(hlffi_vm* vm){ if(!vm) return; if(vm->entry) free(vm->entry); free(vm); }
bool hlffi_ready(hlffi_vm* vm){ return vm && vm->ready; }

/* ---- 11.4  Low‑level lookup + call ------------------------------------ */
void* hlffi_lookup(hlffi_vm* vm, const char* name, int nargs){ if(!vm || !vm->mod) return NULL; int fid = hl_hash_utf8(name); return (void*)hl_module_lookup(vm->mod, fid, nargs); }

#define HLFFI_CALL_WRAP(N, CALL)                          \
    int32_t hlffi_call##N(void* f, ...) {                \
        if(!f) return 0;                                \
        va_list vl; va_start(vl, f);                    \
        int32_t ret = 0;                               \
        switch(N){                                      \
            case 0: ret=(int32_t)(intptr_t)hl_call0((vclosure*)f); break;            \
            case 1: ret=(int32_t)(intptr_t)hl_call1((vclosure*)f, va_arg(vl, void*)); break; \
            case 2: ret=(int32_t)(intptr_t)hl_call2((vclosure*)f, va_arg(vl, void*), va_arg(vl, void*)); break; \
            case 3: ret=(int32_t)(intptr_t)hl_call3((vclosure*)f, va_arg(vl, void*), va_arg(vl, void*), va_arg(vl, void*)); break; \
            case 4: ret=(int32_t)(intptr_t)hl_call4((vclosure*)f, va_arg(vl, void*), va_arg(vl, void*), va_arg(vl, void*), va_arg(vl, void*)); break; }
        va_end(vl);                                     \
        return ret; }
HLFFI_CALL_WRAP(0, hl_call0)
HLFFI_CALL_WRAP(1, hl_call1)
HLFFI_CALL_WRAP(2, hl_call2)
HLFFI_CALL_WRAP(3, hl_call3)
HLFFI_CALL_WRAP(4, hl_call4)
#undef HLFFI_CALL_WRAP

int32_t hlffi_call_variadic(void* f, int argc, ...){ if(!f) return 0; va_list vl; va_start(vl, argc); vdynamic** argv = (vdynamic**)alloca(sizeof(vdynamic*) * argc); for(int i=0;i<argc;i++){ argv[i] = (vdynamic*)va_arg(vl, void*); } va_end(vl); return (int32_t)(intptr_t)hl_callN((vclosure*)f, argc, argv); }

/* ---- 11.5  Extended helpers ------------------------------------------- */
#if HLFFI_EXT_STATIC || HLFFI_EXT_VAR
static hl_type* find_class(hlffi_vm* vm, const char* cls){ if(!vm || !vm->code) return NULL; for(int i=0;i<vm->code->ntypes;i++){ hl_type* t = &vm->code->types[i]; if(t->kind == HOBJ && t->obj->name){ char* utf = hl_to_utf8((uchar*)t->obj->name); if(utf && strcmp(utf, cls)==0) return t; } } return NULL; }
#endif

#if HLFFI_EXT_STATIC
static vdynamic* call_static_dyn(hlffi_vm* vm, hl_type* tp, const char* fn){ vdynamic* g = *(vdynamic**)tp->obj->global_value; if(!g) return NULL; int h = hl_hash_utf8(fn); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return NULL; vclosure* cl = (vclosure*)hl_dyn_getp(g, l->hashed_name, &hlt_dyn); if(!cl) return NULL; return hl_dyn_call(cl, NULL, 0); }
bool hlffi_call_static(hlffi_vm* vm, const char* cls, const char* fn){ hl_type* t = find_class(vm, cls); if(!t) return false; return call_static_dyn(vm, t, fn) != NULL; }
int hlffi_call_static_ret_int(hlffi_vm* vm, const char* cls, const char* fn){ hl_type* t = find_class(vm, cls); if(!t) return 0; vdynamic* r = call_static_dyn(vm, t, fn); return (r && r->t == &hlt_i32) ? r->v.i : 0; }
double hlffi_call_static_ret_float(hlffi_vm* vm, const char* cls, const char* fn){ hl_type* t = find_class(vm, cls); if(!t) return 0.0; vdynamic* r = call_static_dyn(vm, t, fn); return (r && r->t == &hlt_f64) ? r->v.d : 0.0; }
char* hlffi_call_static_ret_string(hlffi_vm* vm, const char* cls, const char* fn){ hl_type* t = find_class(vm, cls); if(!t) return NULL; vdynamic* r = call_static_dyn(vm, t, fn); if(!r) return NULL; char* s = hl_to_utf8(hl_to_string(r)); return s ? strdup(s) : NULL; }
/*  one‑arg flavours (int/float/string) */
bool hlffi_call_static_int(hlffi_vm* vm, const char* cls, const char* fn, int v){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(fn); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; vclosure* cl = (vclosure*)hl_dyn_getp(g, l->hashed_name, &hlt_dyn); if(!cl) return false; vdynamic* arg = hl_alloc_dynamic(&hlt_i32); arg->v.i = v; vdynamic* argv[1] = { arg }; return hl_dyn_call(cl, argv, 1) != NULL; }
bool hlffi_call_static_float(hlffi_vm* vm, const char* cls, const char* fn, double v){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(fn); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; vclosure* cl = (vclosure*)hl_dyn_getp(g, l->hashed_name, &hlt_dyn); if(!cl) return false; vdynamic* arg = hl_alloc_dynamic(&hlt_f64); arg->v.d = v; vdynamic* argv[1] = { arg }; return hl_dyn_call(cl, argv, 1) != NULL; }
bool hlffi_call_static_string(hlffi_vm* vm, const char* cls, const char* fn, const char* s){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(fn); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; vclosure* cl = (vclosure*)hl_dyn_getp(g, l->hashed_name, &hlt_dyn); if(!cl) return false; uchar* us = hl_from_utf8((char*)s); vdynamic* arg = hl_alloc_string(us); vdynamic* argv[1] = { arg }; return hl_dyn_call(cl, argv, 1) != NULL; }
#endif /* EXT_STATIC */

#if HLFFI_EXT_VAR
static vdynamic* get_static_dyn(hlffi_vm* vm, const char* cls, const char* var){ hl_type* t = find_class(vm, cls); if(!t) return NULL; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(var); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return NULL; return (vdynamic*)hl_dyn_getp(g, l->hashed_name, &hlt_dyn); }
int hlffi_get_static_int(hlffi_vm* vm, const char* cls, const char* var){ vdynamic* v = get_static_dyn(vm, cls, var); return (v && v->t == &hlt_i32) ? v->v.i : 0; }
double hlffi_get_static_float(hlffi_vm* vm, const char* cls, const char* var){ vdynamic* v = get_static_dyn(vm, cls, var); return (v && v->t == &hlt_f64) ? v->v.d : 0.0; }
char* hlffi_get_static_string(hlffi_vm* vm, const char* cls, const char* var){ vdynamic* v = get_static_dyn(vm, cls, var); if(!v) return NULL; char* s = hl_to_utf8(hl_to_string(v)); return s ? strdup(s) : NULL; }
bool hlffi_set_static_int(hlffi_vm* vm, const char* cls, const char* var, int v){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(var); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; vdynamic* dyn = hl_alloc_dynamic(&hlt_i32); dyn->v.i = v; hl_dyn_setp(g, l->hashed_name, &hlt_dyn, dyn); return true; }
bool hlffi_set_static_float(hlffi_vm* vm, const char* cls, const char* var, double v){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(var); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; vdynamic* dyn = hl_alloc_dynamic(&hlt_f64); dyn->v.d = v; hl_dyn_setp(g, l->hashed_name, &hlt_dyn, dyn); return true; }
bool hlffi_set_static_string(hlffi_vm* vm, const char* cls, const char* var, const char* s){ hl_type* t = find_class(vm, cls); if(!t) return false; vdynamic* g = *(vdynamic**)t->obj->global_value; int h = hl_hash_utf8(var); hl_field_lookup* l = hl_obj_resolve_field(g->t->obj, h); if(!l) return false; uchar* us = hl_from_utf8((char*)s); vdynamic* dyn = hl_alloc_string(us); hl_dyn_setp(g, l->hashed_name, &hlt_dyn, dyn); return true; }
#endif /* EXT_VAR */

#if HLFFI_EXT_VALUE
struct hlffi_value { hlffi_val_kind k; union { int i; double d; bool b; char* s; void* p; }; vdynamic* dyn; };
static hlffi_value* box_val(hlffi_val_kind k){ hlffi_value* v = (hlffi_value*)malloc(sizeof(hlffi_value)); v->k = k; v->dyn = NULL; return v; }
hlffi_value* hlffi_val_int(int i){ hlffi_value* v = box_val(HLFFI_T_INT); v->i = i; v->dyn = hl_alloc_dynamic(&hlt_i32); v->dyn->v.i = i; return v; }
hlffi_value* hlffi_val_float(double d){ hlffi_value* v = box_val(HLFFI_T_DOUBLE); v->d = d; v->dyn = hl_alloc_dynamic(&hlt_f64); v->dyn->v.d = d; return v; }
hlffi_value* hlffi_val_bool(bool b){ hlffi_value* v = box_val(HLFFI_T_BOOL); v->b = b; v->dyn = hl_alloc_dynamic(&hlt_bool); v->dyn->v.b = b; return v; }
hlffi_value* hlffi_val_string(const char* s){ hlffi_value* v = box_val(HLFFI_T_STRING); v->s = s ? strdup(s) : NULL; uchar* us = hl_from_utf8((char*)s); v->dyn = hl_alloc_string(us); return v; }
hlffi_value* hlffi_val_null(void){ return box_val(HLFFI_T_NULL); }
void hlffi_val_free(hlffi_value* v){ if(!v) return; if(v->k == HLFFI_T_STRING && v->s) free(v->s); free(v); }
hlffi_val_kind hlffi_val_type(hlffi_value* v){ return v ? v->k : HLFFI_T_NULL; }
int hlffi_val_as_int(hlffi_value* v){ if(!v) return 0; switch(v->k){ case HLFFI_T_INT: return v->i; case HLFFI_T_DOUBLE: return (int)v->d; case HLFFI_T_BOOL: return v->b ? 1 : 0; default: return 0;} }
double hlffi_val_as_float(hlffi_value* v){ if(!v) return 0.0; switch(v->k){ case HLFFI_T_DOUBLE: return v->d; case HLFFI_T_INT: return (double)v->i; case HLFFI_T_BOOL: return v->b ? 1.0 : 0.0; default: return 0.0;} }
bool hlffi_val_as_bool(hlffi_value* v){ if(!v) return false; switch(v->k){ case HLFFI_T_BOOL: return v->b; case HLFFI_T_INT: return v->i != 0; case HLFFI_T_DOUBLE: return v->d != 0.0; case HLFFI_T_STRING: return v->s && *v->s; default: return v->p != NULL; } }
char* hlffi_val_as_string(hlffi_value* v){ if(!v) return NULL; switch(v->k){ case HLFFI_T_STRING: return v->s ? strdup(v->s) : NULL; case HLFFI_T_INT:{ char* buf = (char*)malloc(32); snprintf(buf, 32, "%d", v->i); return buf; } case HLFFI_T_DOUBLE:{ char* buf = (char*)malloc(32); snprintf(buf, 32, "%g", v->d); return buf; } case HLFFI_T_BOOL: return strdup(v->b ? "true" : "false"); default: return NULL; } }
#endif /* EXT_VALUE */

/* ---- 11.6  Thread & GC -------------------------------------------------- */
void hlffi_thread_attach(void){ hl_register_thread(NULL); }
void hlffi_thread_detach(void){ hl_unregister_thread(); }
void hlffi_gc_stats(uint32_t* objs, uint64_t* heap){ int o=0; double m=0.0; hl_gc_info(&o,&m); if(objs) *objs=(uint32_t)o; if(heap) *heap=(uint64_t)m; }
void hlffi_gc_collect(void){ hl_gc_major(); }

/* ---- 11.7  Version ------------------------------------------------------ */
const char* hlffi_version(void){ return HLFFI_VER_STR; }

#endif /* HLFFI_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HLFFI_H */
