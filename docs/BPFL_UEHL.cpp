#pragma once
#include "BPFL_UEHL.h"
#include <hlMain.h>

bool UBPFL_UEHL::HL_Start_VM(FString path)
{
	FString absPAth = FPaths::ProjectDir() + "/" + path;
	int ret = hl_start_vm(TCHAR_TO_ANSI(*absPAth));
	return ret == 0;
}

void UBPFL_UEHL::HL_Stop_VM()
{
	hl_stop_vm();
}

bool UBPFL_UEHL::HL_RunStaticMethodOnClassByName(FString className, FString classMethod)
{
	vdynamic* ret = hl_run_static_method_on_static_class_by_name(TCHAR_TO_ANSI(*className), TCHAR_TO_ANSI(*classMethod));
	return true;
}
