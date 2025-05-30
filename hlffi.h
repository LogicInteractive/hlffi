/*
   hlffi.h — HashLink FFI one‑header bridge (v2.2.2)
   =================================================
   A single‑file header that embeds the HashLink VM (Haxe) into any C/C++
   application — engines, plugins, CLI tools, you name it.

   ┌──────────────────────────── HOW TO USE ────────────────────────────┐
   │   //  In **one** .c/.cpp file **only**                             │
   │   #define HLFFI_IMPLEMENTATION                                     │
   │   #include "hlffi.h"                                               │
   │                                                                    │
   │   //  (Optional) feature flags *before* the include                │
   │   #define HLFFI_EXT_STATIC 0                                       │
   └────────────────────────────────────────────────────────────────────┘
*/

#ifndef HLFFI_H
#define HLFFI_H
#pragma once

/* ============================= 0.  INCLUDES ============================= */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

/*  HashLink public headers  */
#include <hl.h>
#if __has_include(<hlmodule.h>)
#  include <hlmodule.h>
#else
#  include "hlmodule.h"
#endif

/* =========================================================================
 * 1.  FEATURE FLAGS                                                        */
#ifndef HLFFI_EXT_STATIC
#  define HLFFI_EXT_STATIC   1
#endif
#ifndef HLFFI_EXT_VAR
#  define HLFFI_EXT_VAR      1
#endif
#ifndef HLFFI_EXT_VALUE
#  define HLFFI_EXT_VALUE    1
#endif
#ifndef HLFFI_EXT_ERRORS
#  define HLFFI_EXT_ERRORS   1
#endif

/* =========================================================================
 * 2.  VERSION & INTERNALS                                                  */
#define HL_STR2(x) #x
#define HL_STR(x) HL_STR2(x)
#define HLFFI_VER_STR "2.2.2 (HL " HL_STR(HL_VERSION) ")"
#define HLFFI_ERRBUF 256

typedef struct hlffi_vm {
    hl_module* mod;
    hl_code*   code;
    vclosure*  entry;
    bool       ready;
    bool       init;
#if HLFFI_EXT_ERRORS
    int        err_code;
    char       err_msg[HLFFI_ERRBUF];
#endif
} hlffi_vm;

/* =========================================================================
 * 3.  PUBLIC API — PROTOTYPES                                              */

/* 3.1  VM life‑cycle */
hlffi_vm*     hlffi_create           (void);
void          hlffi_close            (hlffi_vm*);
void          hlffi_destroy          (hlffi_vm*);
bool          hlffi_ready            (hlffi_vm*);

/* 3.2  Sys init (argc/argv) */
bool          hlffi_init_args        (hlffi_vm*, int argc, const char** argv);

/* 3.3  Load .hl code */
#if HLFFI_EXT_ERRORS
typedef enum { HLFFI_OK=0, HLFFI_E_INIT, HLFFI_E_LOAD, HLFFI_E_RUN } hlffi_result;
hlffi_result  hlffi_load_file        (hlffi_vm*, const char* path);
hlffi_result  hlffi_load_mem         (hlffi_vm*, const void* data, int size);
hlffi_result  hlffi_call_entry       (hlffi_vm*);
const char*   hlffi_error            (hlffi_vm*);
#else
bool          hlffi_load_file_bool   (hlffi_vm*, const char* path, const char** errmsg);
bool          hlffi_load_mem_bool    (hlffi_vm*, const void* data, int size, const char** errmsg);
bool          hlffi_call_entry_bool  (hlffi_vm*);
#endif

/* 3.4  Lookup + generic calls */
void*         hlffi_lookup           (hlffi_vm*, const char* name, int nargs);
vdynamic*     hlffi_call0            (void* f);
vdynamic*     hlffi_call1            (void* f, vdynamic* a0);
vdynamic*     hlffi_call2            (void* f, vdynamic* a0, vdynamic* a1);
vdynamic*     hlffi_call3            (void* f, vdynamic* a0, vdynamic* a1, vdynamic* a2);
vdynamic*     hlffi_call4            (void* f, vdynamic* a0, vdynamic* a1, vdynamic* a2, vdynamic* a3);
vdynamic*     hlffi_call_variadic    (void* f, int argc, ...);

/* 3.5  Static helpers (syntactic sugar) */
#if HLFFI_EXT_STATIC
bool          hlffi_call_static            (hlffi_vm*, const char* cls, const char* fun);
bool          hlffi_call_static_int        (hlffi_vm*, const char* cls, const char* fun, int);
bool          hlffi_call_static_float      (hlffi_vm*, const char* cls, const char* fun, double);
bool          hlffi_call_static_string     (hlffi_vm*, const char* cls, const char* fun, const char*);
int           hlffi_call_static_ret_int    (hlffi_vm*, const char* cls, const char* fun);
double        hlffi_call_static_ret_float  (hlffi_vm*, const char* cls, const char* fun);
const char*   hlffi_call_static_ret_string (hlffi_vm*, const char* cls, const char* fun);
#endif

/* 3.6  Boxing helpers */
#if HLFFI_EXT_VALUE
typedef enum { HLFFI_T_NULL, HLFFI_T_INT, HLFFI_T_FLOAT, HLFFI_T_BOOL, HLFFI_T_STRING, HLFFI_T_PTR } hlffi_val_kind;
typedef struct {
    hlffi_val_kind k;
    union { int i; double d; bool b; void* p; const char* s; };
    vdynamic* dyn;
} hlffi_value;

hlffi_value* hlffi_val_int      (int32_t);
hlffi_value* hlffi_val_float    (double);
hlffi_value* hlffi_val_bool     (bool);
hlffi_value* hlffi_val_string   (const char*);
hlffi_value* hlffi_val_null     (void);
void         hlffi_val_free     (hlffi_value*);
int          hlffi_val_as_int   (hlffi_value*);
double       hlffi_val_as_float (hlffi_value*);
bool         hlffi_val_as_bool  (hlffi_value*);
const char*  hlffi_val_as_string(hlffi_value*);
#endif

/* 3.7  Thread & GC helpers */
void          hlffi_thread_attach (void);
void          hlffi_thread_detach (void);
void          hlffi_gc_stats      (uint32_t* objs, uint64_t* heap);

/* 3.8  Misc */
const char*   hlffi_version       (void);

/* --------------------------------------------------------------------- */
static inline int32_t hlffi_dyn_to_i32 (vdynamic* v){ return (v&&v->t==&hlt_i32)  ? v->v.i : 0; }
static inline double  hlffi_dyn_to_f64 (vdynamic* v){ return (v&&v->t==&hlt_f64)  ? v->v.d : 0.0; }
static inline bool    hlffi_dyn_to_bool(vdynamic* v){ return (v&&v->t==&hlt_bool) ? v->v.b : false; }
static inline const char* hlffi_dyn_to_cstr(vdynamic* v){ return v ? hl_to_utf8(hl_to_string(v)) : NULL; }

/* =========================================================================
 * 4.  IMPLEMENTATION                                                      */
#ifdef HLFFI_IMPLEMENTATION

/* ---- 4.1  VM life‑cycle ---------------------------------------------- */
hlffi_vm* hlffi_create(void){
    hlffi_vm* vm = (hlffi_vm*)calloc(1,sizeof(hlffi_vm));
    if(!vm) return NULL;
    hl_global_init();
    hl_register_thread(NULL);
    vm->init = true;
    return vm;
}

void hlffi_close(hlffi_vm* vm){
    if(!vm || !vm->init) return;
    hl_global_free();
    vm->init = vm->ready = false;
}

void hlffi_destroy(hlffi_vm* vm){ if(vm) free(vm); }

bool hlffi_ready(hlffi_vm* vm){ return vm && vm->ready; }

/* ---- 4.2  Sys init ---------------------------------------------------- */
bool hlffi_init_args(hlffi_vm* vm, int argc, const char** argv){
    if(!vm||!vm->init) return false;
    hl_sys_init((void*)argv,argc,NULL);
    return true;
}

/* ---- 4.3  Load helpers ------------------------------------------------- */
#if HLFFI_EXT_ERRORS
#define ERR(VM,CODE,FMT,...) do{ if(VM){ (VM)->err_code=CODE; snprintf((VM)->err_msg,HLFFI_ERRBUF,FMT,__VA_ARGS__);} return CODE; }while(0)
#else
#define ERR_BOOL(R,MSG,...) do{ if(MSG) *MSG=""; return R; }while(0)
#endif

static bool build_entry_closure(hlffi_vm* vm){
    int ep = vm->code->entrypoint;
    int pf = vm->mod->functions_indexes[ep];
    hl_type* tp = vm->code->functions[ep].type;
    void* fptr = vm->mod->functions_ptrs[pf];
    vm->entry = hl_alloc_closure_void(tp,fptr);
    return vm->entry!=NULL;
}

static bool load_code_blob(hlffi_vm* vm,const unsigned char* buf,size_t len){
    if(!hl_code_read(buf,len,&vm->code)) return false;
    vm->mod = hl_module_alloc(vm->code);
    if(!vm->mod) return false;
    if(!hl_module_init(vm->mod,NULL,NULL)) return false;
    if(!build_entry_closure(vm)) return false;
    vm->ready = true;
    return true;
}

#if HLFFI_EXT_ERRORS
hlffi_result hlffi_load_file(hlffi_vm* vm,const char* path){
    if(!vm||!vm->init) return HLFFI_E_INIT;
    FILE* fp=fopen(path,"rb"); if(!fp) ERR(vm,HLFFI_E_LOAD,"open '%s' failed",path);
    fseek(fp,0,SEEK_END); long len=ftell(fp); fseek(fp,0,SEEK_SET);
    unsigned char* buf=(unsigned char*)malloc(len);
    if(fread(buf,1,len,fp)!=(size_t)len){ fclose(fp); free(buf); ERR(vm,HLFFI_E_LOAD,"read '%s' failed",path); }
    fclose(fp);
    bool ok=load_code_blob(vm,buf,len);
    free(buf);
    return ok?HLFFI_OK:HLFFI_E_LOAD;
}

hlffi_result hlffi_load_mem(hlffi_vm* vm,const void* data,int size){
    if(!vm||!vm->init) return HLFFI_E_INIT;
    return load_code_blob(vm,(const unsigned char*)data,size)?HLFFI_OK:HLFFI_E_LOAD;
}

hlffi_result hlffi_call_entry(hlffi_vm* vm){
    if(!vm||!vm->ready) return HLFFI_E_INIT;
    hl_call0(vm->entry);
    return HLFFI_OK;
}

const char* hlffi_error(hlffi_vm* vm){ return vm?vm->err_msg:""; }
#else
bool hlffi_load_file_bool(hlffi_vm* vm,const char* path,const char** err){
    if(!vm||!vm->init) return false;
    FILE* fp=fopen(path,"rb"); if(!fp) ERR_BOOL(false,err);
    fseek(fp,0,SEEK_END); long len=ftell(fp); fseek(fp,0,SEEK_SET);
    unsigned char* buf=(unsigned char*)malloc(len);
    if(fread(buf,1,len,fp)!=(size_t)len){ fclose(fp); free(buf); ERR_BOOL(false,err); }
    fclose(fp);
    bool ok=load_code_blob(vm,buf,len); free(buf); return ok;
}

bool hlffi_load_mem_bool(hlffi_vm* vm,const void* data,int size,const char** err){
    if(!vm||!vm->init) return false;
    return load_code_blob(vm,(const unsigned char*)data,size);
}

bool hlffi_call_entry_bool(hlffi_vm* vm){ if(!vm||!vm->ready) return false; hl_call0(vm->entry); return true; }
#endif

/* ---- 4.4  Lookup + calls --------------------------------------------- */
void* hlffi_lookup(hlffi_vm* vm,const char* name,int nargs){
    if(!vm||!vm->mod||!name) return NULL;
    int fid=hl_hash_utf8(name);
    for(int i=0;i<vm->code->nfunctions;i++){
        hl_function* f=&vm->code->functions[i];
        if(f->name==fid && f->nargs==nargs){ int idx=vm->mod->functions_indexes[i]; return vm->mod->functions_ptrs[idx]; }
    }
    return NULL;
}

static inline vdynamic* hlffi_call0(void* f){ return f?hl_call0((vclosure*)f):NULL; }
static inline vdynamic* hlffi_call1(void* f,vdynamic* a0){ return f?hl_call1((vclosure*)f,a0):NULL; }
static inline vdynamic* hlffi_call2(void* f,vdynamic* a0,vdynamic* a1){ return f?hl_call2((vclosure*)f,a0,a1):NULL; }
static inline vdynamic* hlffi_call3(void* f,vdynamic* a0,vdynamic* a1,vdynamic* a2){ return f?hl_call3((vclosure*)f,a0,a1,a2):NULL; }
static inline vdynamic* hlffi_call4(void* f,vdynamic* a0,vdynamic* a1,vdynamic* a2,vdynamic* a3){ return f?hl_call4((vclosure*)f,a0,a1,a2,a3):NULL; }

vdynamic* hlffi_call_variadic(void* f,int argc,...){
    if(!f||argc<0) return NULL;
    const int STACK_MAX=16;
    vdynamic* arg_stack[STACK_MAX];
    vdynamic** argv=arg_stack; vdynamic** heap=NULL;
    if(argc>STACK_MAX){ heap=(vdynamic**)malloc(sizeof(vdynamic*)*argc); if(!heap) return NULL; argv=heap; }
    va_list vl; va_start(vl,argc);
    for(int i=0;i<argc;i++) argv[i]=va_arg(vl,vdynamic*);
    va_end(vl);
    vdynamic* ret=hl_callN((vclosure*)f,argc,argv);
    if(heap) free(heap);
    return ret;
}

/* ---- 4.5  Static helpers --------------------------------------------- */
#if HLFFI_EXT_STATIC || HLFFI_EXT_VAR
static hl_type* find_class(hlffi_vm* vm,const char* cls){
    if(!vm||!vm->mod) return NULL;
    for(int i=0;i<vm->code->ntypes;i++){
        hl_type* t=vm->code->types[i];
        if(t->kind==HDK_OBJECT && t->obj->super==NULL){
            const char* utf=hl_to_utf8(t->obj->name);
            if(utf && strcmp(utf,cls)==0) return t;
        }
    }
    return NULL;
}
#endif

#if HLFFI_EXT_STATIC
static vdynamic* call_static_dyn(hlffi_vm* vm,hl_type* tp,const char* fn){
    if(!tp) return NULL;
    vdynamic* cl=hl_alloc_dynamic(tp);
    vdynamic* f=hl_dyn_get(cl,fn);
    return f?hl_dyn_call(f,NULL,0):NULL;
}

bool hlffi_call_static(hlffi_vm* vm,const char* cls,const char* fn){
    return call_static_dyn(vm,find_class(vm,cls),fn)!=NULL;
}

int hlffi_call_static_ret_int(hlffi_vm* vm,const char* cls,const char* fn){
    return hlffi_dyn_to_i32(call_static_dyn(vm,find_class(vm,cls),fn));
}

double hlffi_call_static_ret_float(hlffi_vm* vm,const char* cls,const char* fn){
    return hlffi_dyn_to_f64(call_static_dyn(vm,find_class(vm,cls),fn));
}

const char* hlffi_call_static_ret_string(hlffi_vm* vm,const char* cls,const char* fn){
    return hlffi_dyn_to_cstr(call_static_dyn(vm,find_class(vm,cls),fn));
}

/* one‑arg variants */
static bool call_static_onearg(hlffi_vm* vm,const char* cls,const char* fn,vdynamic* arg){
    hl_type* tp=find_class(vm,cls); if(!tp) return false;
    vdynamic* cl=hl_alloc_dynamic(tp);
    vdynamic* f=hl_dyn_get(cl,fn); if(!f) return false;
    vdynamic* argv[1]={arg};
    return hl_dyn_call(f,argv,1)!=NULL;
}
bool hlffi_call_static_int(hlffi_vm* vm,const char* cls,const char* fn,int arg){ return call_static_onearg(vm,cls,fn,hlffi_val_int(arg)->dyn); }
bool hlffi_call_static_float(hlffi_vm* vm,const char* cls,const char* fn,double arg){ return call_static_onearg(vm,cls,fn,hlffi_val_float(arg)->dyn); }
bool hlffi_call_static_string(hlffi_vm* vm,const char* cls,const char* fn,const char* arg){ return call_static_onearg(vm,cls,fn,hlffi_val_string(arg)->dyn); }
#endif /* HLFFI_EXT_STATIC */

/* ---- 4.6  Boxing helpers --------------------------------------------- */
#if HLFFI_EXT_VALUE
static hlffi_value* box_val(hlffi_val_kind k){ hlffi_value* v=(hlffi_value*)calloc(1,sizeof(hlffi_value)); v->k=k; return v; }

hlffi_value* hlffi_val_int(int32_t i){ hlffi_value* v=box_val(HLFFI_T_INT); v->i=i; v->dyn=hl_alloc_dynamic(&hlt_i32); v->dyn->v.i=i; return v; }
hlffi_value* hlffi_val_float(double d){ hlffi_value* v=box_val(HLFFI_T_FLOAT); v->d=d; v->dyn=hl_alloc_dynamic(&hlt_f64); v->dyn->v.d=d; return v; }
hlffi_value* hlffi_val_bool(bool b){ hlffi_value* v=box_val(HLFFI_T_BOOL); v->b=b; v->dyn=hl_alloc_dynamic(&hlt_bool); v->dyn->v.b=b; return v; }
hlffi_value* hlffi_val_string(const char* s){ hlffi_value* v=box_val(HLFFI_T_STRING); v->s=strdup(s); v->dyn=hl_alloc_string(hl_from_utf8((char*)s)); return v; }
hlffi_value* hlffi_val_null(void){ return box_val(HLFFI_T_NULL); }
void hlffi_val_free(hlffi_value* v){ if(!v) return; if(v->k==HLFFI_T_STRING && v->s) free((void*)v->s); free(v); }
int hlffi_val_as_int(hlffi_value* v){ return v ? (v->k==HLFFI_T_INT ? v->i : hlffi_dyn_to_i32(v->dyn)) : 0; }
double hlffi_val_as_float(hlffi_value* v){ return v ? (v->k==HLFFI_T_FLOAT ? v->d : hlffi_dyn_to_f64(v->dyn)) : 0.0; }
bool hlffi_val_as_bool(hlffi_value* v){ return v ? (v->k==HLFFI_T_BOOL ? v->b : hlffi_dyn_to_bool(v->dyn)) : false; }
const char* hlffi_val_as_string(hlffi_value* v){ return v ? (v->k==HLFFI_T_STRING ? v->s : hlffi_dyn_to_cstr(v->dyn)) : NULL; }
#endif /* HLFFI_EXT_VALUE */

/* ---- 4.7  Thread & GC helpers ---------------------------------------- */
void hlffi_thread_attach(void){ hl_register_thread(NULL); }
void hlffi_thread_detach(void){ hl_unregister_thread(); }
void hlffi_gc_stats(uint32_t* objs,uint64_t* heap){ int o=0; uint64_t h=0; hl_gc_info(&o,&h); if(objs) *objs=(uint32_t)o; if(heap) *heap=h; }

/* ---- 4.8  Misc -------------------------------------------------------- */
const char* hlffi_version(void){ return HLFFI_VER_STR; }

#endif /* HLFFI_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HLFFI_H */
