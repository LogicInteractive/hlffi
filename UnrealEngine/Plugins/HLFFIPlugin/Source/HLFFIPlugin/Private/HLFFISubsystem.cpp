// Copyright LogicInteractive. All Rights Reserved.

#include "HLFFISubsystem.h"
#include "HLFFIPluginModule.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// Include HLFFI header (implementation is in HLFFIPluginModule.cpp)
#include "hlffi.h"

UHLFFISubsystem::UHLFFISubsystem()
{
	VM = nullptr;
	bHotReloadEnabled = true;
	bIsInitializing = false;
}

void UHLFFISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogHLFFI, Log, TEXT("HLFFISubsystem initialized."));

	// Note: We don't automatically start the VM here.
	// Users should call StartVM() with their .hl file path,
	// either from Blueprint BeginPlay or from game code.
}

void UHLFFISubsystem::Deinitialize()
{
	UE_LOG(LogHLFFI, Log, TEXT("HLFFISubsystem deinitializing..."));

	StopVM();

	Super::Deinitialize();
}

bool UHLFFISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create if the HLFFI module is available
	return FHLFFIPluginModule::IsAvailable();
}

void UHLFFISubsystem::Tick(float DeltaTime)
{
	if (VM && !bIsInitializing)
	{
		// Process HLFFI events (UV loop, Haxe EventLoop)
		hlffi_update(VM, DeltaTime);

		// Check for hot reload if enabled
		if (bHotReloadEnabled)
		{
			if (hlffi_check_reload(VM))
			{
				UE_LOG(LogHLFFI, Log, TEXT("Hot reload detected and applied."));
				OnHotReload.Broadcast(true);
			}
		}
	}
}

bool UHLFFISubsystem::IsTickable() const
{
	return VM != nullptr && !bIsInitializing;
}

TStatId UHLFFISubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHLFFISubsystem, STATGROUP_Tickables);
}

// ==================== VM Lifecycle ====================

bool UHLFFISubsystem::StartVM(const FString& HLFilePath)
{
	if (VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("VM is already running. Call StopVM() first or use RestartVM()."));
		return false;
	}

	FString ResolvedPath = ResolveHLFilePath(HLFilePath);

	if (!FPaths::FileExists(ResolvedPath))
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: .hl file not found: %s"), *ResolvedPath);
		return false;
	}

	bIsInitializing = true;

	UE_LOG(LogHLFFI, Log, TEXT("Starting HLFFI VM with: %s"), *ResolvedPath);

	// Step 1: Create VM
	VM = hlffi_create();
	if (!VM)
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Failed to create VM."));
		bIsInitializing = false;
		return false;
	}

	// Step 2: Initialize VM
	hlffi_error_code err = hlffi_init(VM, 0, nullptr);
	if (err != HLFFI_OK)
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Failed to initialize VM: %s"), ANSI_TO_TCHAR(hlffi_get_error(VM)));
		CleanupVM();
		bIsInitializing = false;
		return false;
	}

	// Step 3: Enable hot reload if desired
	if (bHotReloadEnabled)
	{
		hlffi_enable_hot_reload(VM, true);
	}

	// Step 4: Load bytecode
	err = hlffi_load_file(VM, TCHAR_TO_ANSI(*ResolvedPath));
	if (err != HLFFI_OK)
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Failed to load bytecode: %s"), ANSI_TO_TCHAR(hlffi_get_error(VM)));
		CleanupVM();
		bIsInitializing = false;
		return false;
	}

	// Step 5: Call entry point (initializes Haxe statics)
	err = hlffi_call_entry(VM);
	if (err != HLFFI_OK)
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Failed to call entry point: %s"), ANSI_TO_TCHAR(hlffi_get_error(VM)));
		CleanupVM();
		bIsInitializing = false;
		return false;
	}

	CurrentHLFilePath = ResolvedPath;
	bIsInitializing = false;

	UE_LOG(LogHLFFI, Log, TEXT("HLFFI VM started successfully."));

	OnVMStarted.Broadcast();

	return true;
}

void UHLFFISubsystem::StopVM()
{
	if (!VM)
	{
		return;
	}

	UE_LOG(LogHLFFI, Log, TEXT("Stopping HLFFI VM..."));

	CleanupVM();

	CurrentHLFilePath.Empty();

	OnVMStopped.Broadcast();

	UE_LOG(LogHLFFI, Log, TEXT("HLFFI VM stopped."));
}

bool UHLFFISubsystem::RestartVM(const FString& HLFilePath)
{
	FString PathToUse = HLFilePath.IsEmpty() ? CurrentHLFilePath : HLFilePath;

	if (PathToUse.IsEmpty())
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Cannot restart VM - no .hl file path specified."));
		return false;
	}

	StopVM();

	// Brief pause for cleanup
	FPlatformProcess::Sleep(0.1f);

	return StartVM(PathToUse);
}

bool UHLFFISubsystem::IsVMRunning() const
{
	return VM != nullptr && !bIsInitializing;
}

// ==================== Hot Reload ====================

void UHLFFISubsystem::SetHotReloadEnabled(bool bEnable)
{
	bHotReloadEnabled = bEnable;

	if (VM)
	{
		hlffi_enable_hot_reload(VM, bEnable);
	}
}

bool UHLFFISubsystem::IsHotReloadEnabled() const
{
	return bHotReloadEnabled;
}

bool UHLFFISubsystem::TriggerHotReload()
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot trigger hot reload - VM not running."));
		return false;
	}

	hlffi_error_code err = hlffi_reload_module(VM, TCHAR_TO_ANSI(*CurrentHLFilePath));

	bool bSuccess = (err == HLFFI_OK);

	if (bSuccess)
	{
		UE_LOG(LogHLFFI, Log, TEXT("HLFFI: Hot reload successful."));
	}
	else
	{
		UE_LOG(LogHLFFI, Error, TEXT("HLFFI: Hot reload failed: %s"), ANSI_TO_TCHAR(hlffi_get_error(VM)));
	}

	OnHotReload.Broadcast(bSuccess);

	return bSuccess;
}

// ==================== Static Method Calls ====================

bool UHLFFISubsystem::CallStaticMethod(const FString& ClassName, const FString& MethodName)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return false;
	}

	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 0, nullptr);

	// For void methods, result may be nullptr which is OK
	// We check for error by looking at the error state
	const char* error = hlffi_get_error(VM);
	if (error && error[0] != '\0')
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		return false;
	}

	return true;
}

bool UHLFFISubsystem::CallStaticMethodInt(const FString& ClassName, const FString& MethodName, int32 Value)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return false;
	}

	hlffi_value* arg = hlffi_value_int(VM, Value);
	if (!arg)
	{
		return false;
	}

	hlffi_value* args[] = { arg };
	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 1, args);

	hlffi_value_free(arg);

	const char* error = hlffi_get_error(VM);
	if (error && error[0] != '\0')
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		return false;
	}

	return true;
}

bool UHLFFISubsystem::CallStaticMethodFloat(const FString& ClassName, const FString& MethodName, float Value)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return false;
	}

	hlffi_value* arg = hlffi_value_float(VM, (double)Value);
	if (!arg)
	{
		return false;
	}

	hlffi_value* args[] = { arg };
	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 1, args);

	hlffi_value_free(arg);

	const char* error = hlffi_get_error(VM);
	if (error && error[0] != '\0')
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		return false;
	}

	return true;
}

bool UHLFFISubsystem::CallStaticMethodString(const FString& ClassName, const FString& MethodName, const FString& Value)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return false;
	}

	hlffi_value* arg = hlffi_value_string(VM, TCHAR_TO_ANSI(*Value));
	if (!arg)
	{
		return false;
	}

	hlffi_value* args[] = { arg };
	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 1, args);

	hlffi_value_free(arg);

	const char* error = hlffi_get_error(VM);
	if (error && error[0] != '\0')
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		return false;
	}

	return true;
}

int32 UHLFFISubsystem::CallStaticMethodReturnInt(const FString& ClassName, const FString& MethodName, int32 DefaultValue)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return DefaultValue;
	}

	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 0, nullptr);

	if (!result)
	{
		const char* error = hlffi_get_error(VM);
		if (error && error[0] != '\0')
		{
			UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		}
		return DefaultValue;
	}

	int32 value = hlffi_value_as_int(result, DefaultValue);
	hlffi_value_free(result);

	return value;
}

float UHLFFISubsystem::CallStaticMethodReturnFloat(const FString& ClassName, const FString& MethodName, float DefaultValue)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return DefaultValue;
	}

	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 0, nullptr);

	if (!result)
	{
		const char* error = hlffi_get_error(VM);
		if (error && error[0] != '\0')
		{
			UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		}
		return DefaultValue;
	}

	float value = (float)hlffi_value_as_float(result, (double)DefaultValue);
	hlffi_value_free(result);

	return value;
}

FString UHLFFISubsystem::CallStaticMethodReturnString(const FString& ClassName, const FString& MethodName)
{
	if (!VM)
	{
		UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Cannot call method - VM not running."));
		return FString();
	}

	hlffi_value* result = hlffi_call_static(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*MethodName), 0, nullptr);

	if (!result)
	{
		const char* error = hlffi_get_error(VM);
		if (error && error[0] != '\0')
		{
			UE_LOG(LogHLFFI, Warning, TEXT("HLFFI: Error calling %s.%s: %s"), *ClassName, *MethodName, ANSI_TO_TCHAR(error));
		}
		return FString();
	}

	char* str = hlffi_value_as_string(result, nullptr);
	FString value = str ? FString(ANSI_TO_TCHAR(str)) : FString();

	if (str)
	{
		free(str);  // hlffi_value_as_string returns strdup'd string
	}

	hlffi_value_free(result);

	return value;
}

// ==================== Static Field Access ====================

int32 UHLFFISubsystem::GetStaticInt(const FString& ClassName, const FString& FieldName, int32 DefaultValue)
{
	if (!VM)
	{
		return DefaultValue;
	}

	hlffi_value* val = hlffi_get_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName));
	if (!val)
	{
		return DefaultValue;
	}

	int32 result = hlffi_value_as_int(val, DefaultValue);
	hlffi_value_free(val);

	return result;
}

bool UHLFFISubsystem::SetStaticInt(const FString& ClassName, const FString& FieldName, int32 Value)
{
	if (!VM)
	{
		return false;
	}

	hlffi_value* val = hlffi_value_int(VM, Value);
	if (!val)
	{
		return false;
	}

	hlffi_error_code err = hlffi_set_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName), val);
	hlffi_value_free(val);

	return err == HLFFI_OK;
}

float UHLFFISubsystem::GetStaticFloat(const FString& ClassName, const FString& FieldName, float DefaultValue)
{
	if (!VM)
	{
		return DefaultValue;
	}

	hlffi_value* val = hlffi_get_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName));
	if (!val)
	{
		return DefaultValue;
	}

	float result = (float)hlffi_value_as_float(val, (double)DefaultValue);
	hlffi_value_free(val);

	return result;
}

bool UHLFFISubsystem::SetStaticFloat(const FString& ClassName, const FString& FieldName, float Value)
{
	if (!VM)
	{
		return false;
	}

	hlffi_value* val = hlffi_value_float(VM, (double)Value);
	if (!val)
	{
		return false;
	}

	hlffi_error_code err = hlffi_set_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName), val);
	hlffi_value_free(val);

	return err == HLFFI_OK;
}

FString UHLFFISubsystem::GetStaticString(const FString& ClassName, const FString& FieldName)
{
	if (!VM)
	{
		return FString();
	}

	hlffi_value* val = hlffi_get_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName));
	if (!val)
	{
		return FString();
	}

	char* str = hlffi_value_as_string(val, nullptr);
	FString result = str ? FString(ANSI_TO_TCHAR(str)) : FString();

	if (str)
	{
		free(str);
	}

	hlffi_value_free(val);

	return result;
}

bool UHLFFISubsystem::SetStaticString(const FString& ClassName, const FString& FieldName, const FString& Value)
{
	if (!VM)
	{
		return false;
	}

	hlffi_value* val = hlffi_value_string(VM, TCHAR_TO_ANSI(*Value));
	if (!val)
	{
		return false;
	}

	hlffi_error_code err = hlffi_set_static_field(VM, TCHAR_TO_ANSI(*ClassName), TCHAR_TO_ANSI(*FieldName), val);
	hlffi_value_free(val);

	return err == HLFFI_OK;
}

// ==================== Utilities ====================

FString UHLFFISubsystem::GetCurrentHLFilePath() const
{
	return CurrentHLFilePath;
}

FString UHLFFISubsystem::GetLastError() const
{
	if (!VM)
	{
		return TEXT("VM not initialized");
	}

	const char* error = hlffi_get_error(VM);
	return error ? FString(ANSI_TO_TCHAR(error)) : FString();
}

void UHLFFISubsystem::ForceGarbageCollection()
{
	if (VM)
	{
		hlffi_gc_collect(VM);
	}
}

// ==================== Private Helpers ====================

FString UHLFFISubsystem::ResolveHLFilePath(const FString& InPath) const
{
	// If already absolute, use as-is
	if (FPaths::IsRelative(InPath) == false)
	{
		return InPath;
	}

	// Try relative to Content folder
	FString ContentPath = FPaths::Combine(FPaths::ProjectContentDir(), InPath);
	if (FPaths::FileExists(ContentPath))
	{
		return FPaths::ConvertRelativePathToFull(ContentPath);
	}

	// Try relative to Project folder
	FString ProjectPath = FPaths::Combine(FPaths::ProjectDir(), InPath);
	if (FPaths::FileExists(ProjectPath))
	{
		return FPaths::ConvertRelativePathToFull(ProjectPath);
	}

	// Return as-is, StartVM will report the error
	return InPath;
}

void UHLFFISubsystem::CleanupVM()
{
	if (VM)
	{
		hlffi_close(VM);
		hlffi_destroy(VM);
		VM = nullptr;
	}
}
