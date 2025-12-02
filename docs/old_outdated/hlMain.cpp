//#include <stdio.h>

//#define HL_NAME(n) builtin_##n

//#define WIN32_LEAN_AND_MEAN 
//#include <Windows.h>
extern "C"
{
	#include <hl.h>
	#include <hlmodule.h>
}


#include <locale.h>
#include <thread>
typedef uchar pchar;
#define pprintf(str,file)	uprintf(USTR(str),file)
#define pfopen(file,ext) _wfopen(file,USTR(ext))
#define pcompare wcscmp
#define ptoi(s)	wcstol(s,NULL,10)
#define PSTR(x) USTR(x)

wchar_t* cstr_conv(const char* cstr)
{
	size_t size = mbstowcs(NULL, cstr, 0) + 1; // Get the required size
	wchar_t* wstr = (wchar_t*)malloc(size * sizeof(wchar_t)); // Allocate memory
	mbstowcs(wstr, cstr, size); // Perform the conversion
	return wstr;
}

int ucs2length(vbyte* str) {
	int length = 0;
	while (str[length] != '\0') {
		length++;
	}
	return length;
}

typedef struct
{
	pchar *file;
	hl_code *code;
	hl_module *m;
	vdynamic *ret;
	int file_time;
} main_context;

static int pfiletime( pchar *file )
{
	struct _stat32 st;
	_wstat32(file,&st);
	return (int)st.st_mtime;
}

static hl_code *load_code( const pchar *file, char **error_msg, bool print_errors )
{
	hl_code *code;
	FILE *f = pfopen(file,"rb");
	int pos, size;
	char *fdata;
	if( f == NULL ) {
		if( print_errors ) pprintf("File not found '%s'\n",file);
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	fdata = (char*)malloc(size);
	pos = 0;
	while( pos < size ) {
		int r = (int)fread(fdata + pos, 1, size-pos, f);
		if( r <= 0 ) {
			if( print_errors ) pprintf("Failed to read '%s'\n",file);
			return NULL;
		}
		pos += r;
	}
	fclose(f);
	code = hl_code_read((unsigned char*)fdata, size, error_msg);
	free(fdata);
	return code;
}

static bool check_reload( main_context *m )
{
	int time = pfiletime(m->file);
	bool changed;
	if( time == m->file_time )
		return false;
	char *error_msg = NULL;
	hl_code *code = load_code(m->file, &error_msg, false);
	if( code == NULL )
		return false;
	changed = hl_module_patch(m->m, code);
	m->file_time = time;
	hl_code_free(code);
	return changed;
}

#ifdef HL_VCC
 //this allows some runtime detection to switch to high performance mode
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static void setup_handler()
{
}

main_context ctx;

/*
vdynamic* callStatic(const char* methodName, hl_type* type)
{
	vdynamic* glbl = *(vdynamic**)type->obj->global_value;
	bool isException = false;
	int hash = hl_hash_utf8(methodName);
	hl_field_lookup* lo = hl_obj_resolve_field(glbl->t->obj, hash);
	vclosure* method = (vclosure*)hl_dyn_getp(glbl, lo->hashed_name, &hlt_dyn);
	vdynamic* result;
	if (method)
		result = hl_dyn_call(method, nullptr, 0);
	return result;
}

hl_type* findClassByName(const char* className)
{
	for (int i = 0; i < ctx.code->ntypes; i++)
	{
		hl_type* type = ctx.code->types + i;

		if (type->kind == HOBJ && ucmp(type->obj->name, cstr_conv(className)) == 0)
		{
			return type;
		}
	}
	return nullptr;
}

vdynamic* callStaticOnClass(const char* className, const char* methodName)
{
	hl_type* classobj = findClassByName(className);
	if (classobj)
		return callStatic(methodName, classobj);
	else
		return nullptr;
}

vdynamic* getStaticVar(const char* varName, hl_type* type)
{
	vdynamic* glbl = *(vdynamic**)type->obj->global_value;
	int hash = hl_hash_utf8(varName);
	hl_field_lookup* lo = hl_obj_resolve_field(glbl->t->obj, hash);
	vdynamic* var = (vdynamic*)hl_dyn_getp(glbl, lo->hashed_name, &hlt_dyn);
	return var;
}

void setStaticInt(const char* varName, hl_type* type, int value)
{
	vdynamic* glbl = *(vdynamic**)type->obj->global_value;
	int hash = hl_hash_utf8(varName);
	hl_field_lookup* lo = hl_obj_resolve_field(glbl->t->obj, hash);

	vdynamic* var = (vdynamic*)hl_dyn_getp(glbl, lo->hashed_name, &hlt_dyn);
	if (var)
	{
		if (var->t->kind == HI32)
		{
			var->v.i = value;
			hl_dyn_setp(glbl, lo->hashed_name, &hlt_dyn, var);
		}
	}
	else
	{
		var = (vdynamic*)hl_gc_alloc_raw(sizeof(vdynamic));
		var->t = &hlt_i32;
		var->v.i = value;
		hl_dyn_setp(glbl, lo->hashed_name, &hlt_dyn, var);
	}
}

//void setStaticFunctionPointer(const char* varName, hl_type* type, vdynamic* value)
void setStaticFunctionPointer(const char* varName, hl_type* type, void (*value)())
{
	//printf("Ok..... %i", value);
	//value();

	vclosure* closure = (vclosure*)hl_gc_alloc_raw(sizeof(vclosure));
	closure->t = &hlt_void; // or the actual function type
	closure->fun = value; // your function
	closure->hasValue = false;

	vdynamic* glbl = *(vdynamic**)type->obj->global_value;
	int hash = hl_hash_utf8(varName);
	hl_field_lookup* lo = hl_obj_resolve_field(glbl->t->obj, hash);

	vdynamic* var = (vdynamic*)hl_dyn_getp(glbl, lo->hashed_name, &hlt_dyn);
	if (var)
	{
		if (var->t->kind == HDYN)
		{
			var->v.ptr = (vdynamic*)value;
			hl_dyn_setp(glbl, lo->hashed_name, &hlt_dyn, var);
		}
	}
	else
	////{
	vdynamic* var = (vdynamic*)hl_gc_alloc_raw(sizeof(vdynamic));
	var->t = &hlt_void;
	var->t->fun = closure->fun;
	var->t->fun->closure = &closure;
	var->t->fun->closure_type = &hlt_dyn;
	var->t->fun->nargs = 0;
	var->t->fun->ret = &hlt_void;
	var->v.ptr = closure;
	hl_dyn_setp(glbl, lo->hashed_name, &hlt_dyn, var);
	}

	vclosure* var = hl_alloc_closure_void(type, fvalue)
	{
		vclosure* c = (vclosure*)hl_gc_alloc_noptr(sizeof(vclosure));
		c->t = &hlt_dyn;
		c->fun = fvalue;
		c->hasValue = 0;
		c->value = NULL;
		return c;
	}

}

vdynamic* getStaticVarFromClass(const char* className, const char* varName)
{
	hl_type* classobj = findClassByName(className);
	if (classobj)
		return getStaticVar(varName, classobj);
	else
		return nullptr;
}

void checkHLVarType(vdynamic* var)
{
	if (var->t->kind == HBOOL) {
		bool value = var->v.b;
		printf("\nBOOL: %i\n", value?0:1);
	}
	else if (var->t->kind == HI32) {
		int value = var->v.i;
		printf("\nINT: %i\n", value);
	}
	else if (var->t->kind == HF64) {
		double value = var->v.f;
		printf("\nFLOAT64: %f\n", value);
	}
	else if (var->t->kind == HBYTES) {
		vbyte* value = var->v.bytes;
		printf("\nBYTES...\n");
	}	
	else if (var->t->kind == HOBJ)
	{
		vbyte* utf16String = var->v.bytes;
		size_t utf16Length = ucs2length(utf16String);

		size_t utf8Length = wcstombs(NULL, (wchar_t*)utf16String, 0);
		char* utf8String = (char*)malloc(utf8Length + 1);
		wcstombs(utf8String, (wchar_t*)utf16String, utf8Length + 1);

		printf("\nSTRING: %s\n", utf8String);

		free(utf8String);  // Don't forget to free the string when you're done with it
	}
	else {
		printf("Hm: %i", var->t->kind);
	}
}

HL_PRIM void HL_NAME(foo)(const char* message, double number)
{
	printf("%s : %f\n",message,number);
}
DEFINE_PRIM(_VOID, foo, _BYTES _F64);

HL_PRIM int HL_NAME(getNumber)()
{
	return 888;
}
DEFINE_PRIM(_I32, getNumber, _NO_ARG);


void mcc()
{
	Sleep(500);
	hl_register_thread(NULL);
	hl_enter_thread_stack(0);

	vdynamic* var = getStaticVarFromClass("Datee", "str");
	checkHLVarType(var);

	Sleep(1000);
	hl_type* type = findClassByName("Main");


	vclosure* closure = (vclosure*)hl_gc_alloc_raw(sizeof(vclosure));
	closure->t = &hlt_void; // or the actual function type
	closure->fun = &test_4; // your function
	closure->hasValue = false;
	setStaticFunctionPointer("functionPointer", type, closure);
	setStaticFunctionPointer("functionPointer", type, &test_3);
	setStaticFunctionPointer("functionPointer", type, &test_3);

	callStaticOnClass("Main", "hello");
	Sleep(1000);
	callStaticOnClass("Main", "fuckme");
	callStaticOnClass("Datee", "rocks");
	Sleep(1000);
	callStaticOnClass("test2.Testee", "boo");

	Sleep(1000);
	hl_type* classobj = findClassByName("Main");
	if (classobj)
		return setStaticInt("numa", classobj, 2347);

	Sleep(5000);
}
*/

//int wmain(int argc, pchar* argv[])
//{
	//std::thread hlThread(start_hashlink);
	//std::thread hlThread(mcc);
	//hlThread.detach();
	//start_hashlink();
	//Sleep(10000);


	//for (int i = 0; i < ctx.code->ntypes; i++)
	//{
	//	hl_type* type = ctx.code->types + i;

	//	if (type->kind == HOBJ && ucmp(type->obj->name, USTR("Main")) == 0)
	//	{
	//		vdynamic* glbl = *(vdynamic**)type->obj->global_value;
	//		//vdynamic* instMain = hl_alloc_obj(type);
	//		//vdynamic* instGlbl = hl_alloc_obj(glbl->t);

	//		bool isException = false;
	//		int hash = hl_hash_utf8("hello");
	//		hl_field_lookup* lo = hl_obj_resolve_field(glbl->t->obj, hash);
	//		vclosure* method = (vclosure*)hl_dyn_getp(glbl, lo->hashed_name, &hlt_dyn);
	//		//hl_dyn_call_safe(method, NULL, 0, &isException);
	//		hl_enter_thread_stack(0);
	//		vdynamic* result = hl_dyn_call(method, nullptr, 0);
	//	}
	//}
	/*
	hl_type* mainType = NULL;

	//hl_type* class_type = hl_get_type("MyClass");

	for (int i = 0; i < ctx.code->ntypes; i++)
	{
		hl_type ht = ctx.code->types[i];
		printf("%i", (int)ht.kind);
		//if (ht)
		//{
		//	hl_runtime_obj* objp = hl_get_obj_proto(&ht);
		//	if (objp)
		//	{
		//		printf("%i", objp->nfields);
		//	}
		//}
		//printf("%s\n", ht.tenum->name);
		//if (strcmp(ctx.code->types[i]->name, "Main") == 0)
		////{
		//	mainType = ctx.code->types[i];
		//	break;
		//}
	}

//	hl_runtime_obj* mainProto = hl_get_obj_proto(mainType);
	*/
	//Sleep(2000);
	//return 0;
//}

//int start_hashlink()
//{
	//static vclosure cl;
	//pchar *file = NULL;
	//char *error_msg = NULL;
	//bool hot_reload = false;
	//int profile_count = -1;
	//bool isExc = false;

	//FILE *fchk;
	//file = PSTR("app.hl");
	//fchk = pfopen(file,"rb");
	//if( fchk == NULL )
	//	return 1;
	//fclose(fchk);

	//hl_global_init();
	//hl_sys_init((void**)NULL,0,file);
	//hl_register_thread(&ctx);

	//ctx.file = file;
	//ctx.code = load_code(file, &error_msg, true);
	//if( ctx.code == NULL )
	//	return 1;
	//ctx.m = hl_module_alloc(ctx.code);
	//if( ctx.m == NULL )
	//	return 2;
	//if( !hl_module_init(ctx.m,hot_reload) )
	//	return 3;
	//if( hot_reload ) {
	//	ctx.file_time = pfiletime(ctx.file);
	//	hl_setup_reload_check(check_reload,&ctx);
	//}
	//hl_code_free(ctx.code);

	//cl.t = ctx.code->functions[ctx.m->functions_indexes[ctx.m->code->entrypoint]].type;
	//cl.fun = ctx.m->functions_ptrs[ctx.m->code->entrypoint];
	//cl.hasValue = 0;
	//setup_handler();
	//hl_profile_setup(profile_count);
	//ctx.ret = hl_dyn_call_safe(&cl,NULL,0,&isExc);
	//hl_profile_end();
	//if( isExc ) {
	//	varray *a = hl_exception_stack();
	//	int i;
	//	uprintf(USTR("Uncaught exception: %s\n"), hl_to_string(ctx.ret));
	//	for(i=0;i<a->size;i++)
	//		uprintf(USTR("Called from %s\n"), hl_aptr(a,uchar*)[i]);
	//	hl_debug_break();
	//	hl_global_free();
	//	return 1;
	//}
	//hl_module_free(ctx.m);
	//hl_free(&ctx.code->alloc);

	// do not call hl_unregister_thread() or hl_global_free will display error 
	// on global_lock if there are threads that are still running (such as debugger)
	//hl_global_free();
//}



int start_hl()
{
	//hl_register_thread(NULL);
	//hl_enter_thread_stack(0);

	static vclosure cl;
	pchar *file = NULL;
	char *error_msg = NULL;
	bool hot_reload = false;
	int profile_count = -1;
	bool isExc = false;

	FILE *fchk;
	//file = PSTR("app.hl");
	file = PSTR("c:/dev/Unreal/hlUnreal/hlUnreal/app.hl");
	fchk = pfopen(file,"rb");
	if( fchk == NULL )
		return 1;
	fclose(fchk);

	hl_global_init();
	hl_sys_init((void**)NULL,0,file);
	hl_register_thread(&ctx);

	ctx.file = file;
	ctx.code = load_code(file, &error_msg, true);
	if( ctx.code == NULL )
		return 1;
	ctx.m = hl_module_alloc(ctx.code);
	if( ctx.m == NULL )
		return 2;
	if( !hl_module_init(ctx.m,hot_reload) )
		return 3;
	if( hot_reload ) {
		ctx.file_time = pfiletime(ctx.file);
		hl_setup_reload_check(check_reload,&ctx);
	}
	hl_code_free(ctx.code);
	//hl_global_lock(true);

	cl.t = ctx.code->functions[ctx.m->functions_indexes[ctx.m->code->entrypoint]].type;
	cl.fun = ctx.m->functions_ptrs[ctx.m->code->entrypoint];
	cl.hasValue = 0;
	setup_handler();
	hl_profile_setup(profile_count);
	ctx.ret = hl_dyn_call_safe(&cl,NULL,0,&isExc);
	hl_profile_end();
	if( isExc ) {
		varray *a = hl_exception_stack();
		int i;
		uprintf(USTR("Uncaught exception: %s\n"), hl_to_string(ctx.ret));
		for(i=0;i<a->size;i++)
			uprintf(USTR("Called from %s\n"), hl_aptr(a,uchar*)[i]);
		hl_debug_break();
		hl_global_free();
		return 1;
	}
	hl_module_free(ctx.m);
	hl_free(&ctx.code->alloc);

	 //do not call hl_unregister_thread() or hl_global_free will display error 
	 //on global_lock if there are threads that are still running (such as debugger)
	hl_global_free();
	return 0;
}

void hl_my_test()
{
	start_hl();
	//std::thread hlThread(start_hl);
	//hlThread.detach();
	//start_hashlink();
	//start_hl();
}
