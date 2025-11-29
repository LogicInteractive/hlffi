// Copyright LogicInteractive. All Rights Reserved.

#include "HLFFIBlueprintLibrary.h"
#include "HLFFISubsystem.h"
#include "HLFFIPluginModule.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UHLFFISubsystem* UHLFFIBlueprintLibrary::GetSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: WorldContextObject is null."));
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Could not get World from context object."));
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Could not get GameInstance."));
		return nullptr;
	}

	return GameInstance->GetSubsystem<UHLFFISubsystem>();
}

// ==================== VM Lifecycle ====================

bool UHLFFIBlueprintLibrary::StartVM(const UObject* WorldContextObject, const FString& HLFilePath)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return false;
	}

	return Subsystem->StartVM(HLFilePath);
}

void UHLFFIBlueprintLibrary::StopVM(const UObject* WorldContextObject)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	if (Subsystem)
	{
		Subsystem->StopVM();
	}
}

bool UHLFFIBlueprintLibrary::IsVMRunning(const UObject* WorldContextObject)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsVMRunning() : false;
}

bool UHLFFIBlueprintLibrary::RestartVM(const UObject* WorldContextObject, const FString& HLFilePath)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return false;
	}

	return Subsystem->RestartVM(HLFilePath);
}

// ==================== Hot Reload ====================

void UHLFFIBlueprintLibrary::SetHotReloadEnabled(const UObject* WorldContextObject, bool bEnable)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	if (Subsystem)
	{
		Subsystem->SetHotReloadEnabled(bEnable);
	}
}

bool UHLFFIBlueprintLibrary::TriggerHotReload(const UObject* WorldContextObject)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->TriggerHotReload() : false;
}

// ==================== Static Method Calls ====================

bool UHLFFIBlueprintLibrary::CallStaticMethod(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethod(ClassName, MethodName) : false;
}

bool UHLFFIBlueprintLibrary::CallStaticMethodInt(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, int32 Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodInt(ClassName, MethodName, Value) : false;
}

bool UHLFFIBlueprintLibrary::CallStaticMethodFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, float Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodFloat(ClassName, MethodName, Value) : false;
}

bool UHLFFIBlueprintLibrary::CallStaticMethodString(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, const FString& Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodString(ClassName, MethodName, Value) : false;
}

int32 UHLFFIBlueprintLibrary::CallStaticMethodReturnInt(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, int32 DefaultValue)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodReturnInt(ClassName, MethodName, DefaultValue) : DefaultValue;
}

float UHLFFIBlueprintLibrary::CallStaticMethodReturnFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, float DefaultValue)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodReturnFloat(ClassName, MethodName, DefaultValue) : DefaultValue;
}

FString UHLFFIBlueprintLibrary::CallStaticMethodReturnString(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->CallStaticMethodReturnString(ClassName, MethodName) : FString();
}

// ==================== Static Field Access ====================

int32 UHLFFIBlueprintLibrary::GetStaticInt(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, int32 DefaultValue)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetStaticInt(ClassName, FieldName, DefaultValue) : DefaultValue;
}

bool UHLFFIBlueprintLibrary::SetStaticInt(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, int32 Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->SetStaticInt(ClassName, FieldName, Value) : false;
}

float UHLFFIBlueprintLibrary::GetStaticFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, float DefaultValue)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetStaticFloat(ClassName, FieldName, DefaultValue) : DefaultValue;
}

bool UHLFFIBlueprintLibrary::SetStaticFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, float Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->SetStaticFloat(ClassName, FieldName, Value) : false;
}

FString UHLFFIBlueprintLibrary::GetStaticString(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetStaticString(ClassName, FieldName) : FString();
}

bool UHLFFIBlueprintLibrary::SetStaticString(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, const FString& Value)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->SetStaticString(ClassName, FieldName, Value) : false;
}

// ==================== Utilities ====================

FString UHLFFIBlueprintLibrary::GetLastError(const UObject* WorldContextObject)
{
	UHLFFISubsystem* Subsystem = GetSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastError() : TEXT("Subsystem not available");
}

UHLFFISubsystem* UHLFFIBlueprintLibrary::GetHLFFISubsystem(const UObject* WorldContextObject)
{
	return GetSubsystem(WorldContextObject);
}
