#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPFL_UEHL.generated.h"

#define FS_CS(str) (ANSICHAR*)StringCast<ANSICHAR>(static_cast<const TCHAR*>(str)).Get()
//#define ANSI_TO_TCHAR(str) (TCHAR*)StringCast<TCHAR>(static_cast<const ANSICHAR*>(str)).Get()
//#define TCHAR_TO_UTF8(str) (ANSICHAR*)FTCHARToUTF8((const TCHAR*)str).Get()
//#define UTF8_TO_TCHAR(str) (TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)str).Get()

UCLASS()
class UBPFL_UEHL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HL")
	static bool HL_Start_VM(FString path);

	UFUNCTION(BlueprintCallable, Category = "HL")
	static void HL_Stop_VM();

	UFUNCTION(BlueprintCallable, Category = "HL")
	static bool HL_RunStaticMethodOnClassByName(FString className, FString classMethod);
};
