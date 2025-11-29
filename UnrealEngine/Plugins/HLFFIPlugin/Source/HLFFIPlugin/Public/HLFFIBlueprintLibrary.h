// Copyright LogicInteractive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HLFFIBlueprintLibrary.generated.h"

class UHLFFISubsystem;

/**
 * HLFFI Blueprint Function Library
 *
 * Provides static Blueprint-callable functions for interacting with the HashLink VM.
 * These are convenience wrappers that automatically find the HLFFISubsystem.
 *
 * Usage in Blueprint:
 *   1. Call "HLFFI Start VM" with your .hl file path
 *   2. Call Haxe functions using "HLFFI Call Static Method" nodes
 *   3. VM automatically stops when play ends
 */
UCLASS()
class HLFFIPLUGIN_API UHLFFIBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== VM Lifecycle ====================

	/**
	 * Start the HashLink VM with the specified .hl file
	 * @param WorldContextObject World context for finding the subsystem
	 * @param HLFilePath Path to .hl file (relative to Content folder, or absolute)
	 * @return True if VM started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Start VM"))
	static bool StartVM(const UObject* WorldContextObject, const FString& HLFilePath);

	/**
	 * Stop the HashLink VM
	 * @param WorldContextObject World context for finding the subsystem
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Stop VM"))
	static void StopVM(const UObject* WorldContextObject);

	/**
	 * Check if the VM is currently running
	 * @param WorldContextObject World context for finding the subsystem
	 * @return True if VM is running
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Is VM Running"))
	static bool IsVMRunning(const UObject* WorldContextObject);

	/**
	 * Restart the VM (useful for full state reset)
	 * @param WorldContextObject World context for finding the subsystem
	 * @param HLFilePath Path to .hl file (empty to use current file)
	 * @return True if restart succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Restart VM"))
	static bool RestartVM(const UObject* WorldContextObject, const FString& HLFilePath = TEXT(""));

	// ==================== Hot Reload ====================

	/**
	 * Enable or disable hot reload
	 * @param WorldContextObject World context for finding the subsystem
	 * @param bEnable True to enable hot reload
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Hot Reload", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Set Hot Reload Enabled"))
	static void SetHotReloadEnabled(const UObject* WorldContextObject, bool bEnable);

	/**
	 * Manually trigger a hot reload
	 * @param WorldContextObject World context for finding the subsystem
	 * @return True if reload succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Hot Reload", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Trigger Hot Reload"))
	static bool TriggerHotReload(const UObject* WorldContextObject);

	// ==================== Static Method Calls ====================

	/**
	 * Call a static method on a Haxe class (no arguments)
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method"))
	static bool CallStaticMethod(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName);

	/**
	 * Call a static method with an integer argument
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value Integer argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method (Int)"))
	static bool CallStaticMethodInt(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, int32 Value);

	/**
	 * Call a static method with a float argument
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value Float argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method (Float)"))
	static bool CallStaticMethodFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, float Value);

	/**
	 * Call a static method with a string argument
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value String argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method (String)"))
	static bool CallStaticMethodString(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, const FString& Value);

	/**
	 * Call a static method and get an integer return value
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param DefaultValue Default value if call fails
	 * @return Return value from Haxe method
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method Return Int"))
	static int32 CallStaticMethodReturnInt(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, int32 DefaultValue = 0);

	/**
	 * Call a static method and get a float return value
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param DefaultValue Default value if call fails
	 * @return Return value from Haxe method
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method Return Float"))
	static float CallStaticMethodReturnFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName, float DefaultValue = 0.0f);

	/**
	 * Call a static method and get a string return value
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @return Return value from Haxe method
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Call Static Method Return String"))
	static FString CallStaticMethodReturnString(const UObject* WorldContextObject, const FString& ClassName, const FString& MethodName);

	// ==================== Static Field Access ====================

	/**
	 * Get an integer static field from a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param DefaultValue Default value if field not found
	 * @return Field value
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Get Static Int"))
	static int32 GetStaticInt(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, int32 DefaultValue = 0);

	/**
	 * Set an integer static field on a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Set Static Int"))
	static bool SetStaticInt(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, int32 Value);

	/**
	 * Get a float static field from a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param DefaultValue Default value if field not found
	 * @return Field value
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Get Static Float"))
	static float GetStaticFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, float DefaultValue = 0.0f);

	/**
	 * Set a float static field on a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Set Static Float"))
	static bool SetStaticFloat(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, float Value);

	/**
	 * Get a string static field from a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @return Field value (empty string if not found)
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Get Static String"))
	static FString GetStaticString(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName);

	/**
	 * Set a string static field on a Haxe class
	 * @param WorldContextObject World context for finding the subsystem
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Set Static String"))
	static bool SetStaticString(const UObject* WorldContextObject, const FString& ClassName, const FString& FieldName, const FString& Value);

	// ==================== Utilities ====================

	/**
	 * Get the last error message from HLFFI
	 * @param WorldContextObject World context for finding the subsystem
	 * @return Error message
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Utilities", meta = (WorldContext = "WorldContextObject", DisplayName = "HLFFI Get Last Error"))
	static FString GetLastError(const UObject* WorldContextObject);

	/**
	 * Get the HLFFI subsystem from a world context
	 * @param WorldContextObject World context
	 * @return HLFFI Subsystem, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Utilities", meta = (WorldContext = "WorldContextObject", DisplayName = "Get HLFFI Subsystem"))
	static UHLFFISubsystem* GetHLFFISubsystem(const UObject* WorldContextObject);

private:
	/** Internal helper to get the subsystem */
	static UHLFFISubsystem* GetSubsystem(const UObject* WorldContextObject);
};
