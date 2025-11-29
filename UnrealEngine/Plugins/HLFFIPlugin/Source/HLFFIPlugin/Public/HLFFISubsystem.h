// Copyright LogicInteractive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Containers/Ticker.h"
#include "HLFFISubsystem.generated.h"

// Forward declarations - avoid including hlffi.h in header
struct hlffi_vm;
struct hlffi_value;
struct hlffi_cached_call;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHLFFIVMStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHLFFIVMStopped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHLFFIHotReload, bool, bSuccess);

/**
 * HLFFI Subsystem
 *
 * Manages the HashLink VM lifecycle within Unreal Engine.
 * The VM starts when a game session begins and stops when it ends.
 *
 * Usage:
 *   1. Place your .hl file in the project's Content folder
 *   2. Set the DefaultHLFilePath property (relative to Content folder)
 *   3. Enable bAutoStartVM to start automatically, or call StartVM() manually
 *
 * Features:
 *   - Automatic VM lifecycle management
 *   - Hot reload support for rapid iteration
 *   - Blueprint and C++ API for calling Haxe functions
 *   - Per-frame update for event loop processing
 */
UCLASS(Config=Game)
class HLFFIPLUGIN_API UHLFFISubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UHLFFISubsystem();

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface

	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

	// ==================== Auto-Start Configuration ====================

	/**
	 * If true, the VM will automatically start when the GameInstance initializes.
	 * Set this in your project's DefaultGame.ini or via Blueprint.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "HLFFI|AutoStart")
	bool bAutoStartVM = false;

	/**
	 * Default path to the .hl file to load on auto-start.
	 * Can be relative to Content folder (e.g., "Scripts/game.hl") or absolute.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "HLFFI|AutoStart")
	FString DefaultHLFilePath = TEXT("Scripts/game.hl");

	// ==================== VM Lifecycle ====================

	/**
	 * Start the HashLink VM with the specified .hl file
	 * @param HLFilePath Path to .hl file (relative to Content folder, or absolute)
	 * @return True if VM started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Lifecycle")
	bool StartVM(const FString& HLFilePath);

	/**
	 * Stop the HashLink VM
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Lifecycle")
	void StopVM();

	/**
	 * Restart the VM with a new .hl file (or reload the same file)
	 * @param HLFilePath Path to .hl file (empty to use current file)
	 * @return True if VM restarted successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Lifecycle")
	bool RestartVM(const FString& HLFilePath = TEXT(""));

	/**
	 * Check if the VM is currently running
	 * @return True if VM is initialized and ready
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Lifecycle")
	bool IsVMRunning() const;

	// ==================== High-Frequency Timer Processing ====================

	/**
	 * Enable high-frequency event processing for precise ms-level timers.
	 *
	 * By default, hlffi_update() is called at frame rate (~60Hz = 16ms precision).
	 * For Haxe timers that need 1-10ms precision, enable this to run a background
	 * thread that processes events at 1ms intervals.
	 *
	 * @param bEnable True to enable high-frequency processing
	 * @param IntervalMs Update interval in milliseconds (default: 1ms)
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Timers")
	void SetHighFrequencyTimerEnabled(bool bEnable, int32 IntervalMs = 1);

	/**
	 * Check if high-frequency timer processing is enabled
	 * @return True if the high-frequency timer thread is running
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Timers")
	bool IsHighFrequencyTimerEnabled() const;

	/**
	 * Get the current high-frequency timer interval
	 * @return Interval in milliseconds
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Timers")
	int32 GetHighFrequencyTimerInterval() const;

	// ==================== Hot Reload ====================

	/**
	 * Enable or disable hot reload
	 * @param bEnable True to enable hot reload
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|HotReload")
	void SetHotReloadEnabled(bool bEnable);

	/**
	 * Check if hot reload is enabled
	 * @return True if hot reload is enabled
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|HotReload")
	bool IsHotReloadEnabled() const;

	/**
	 * Manually trigger a hot reload
	 * @return True if reload was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|HotReload")
	bool TriggerHotReload();

	// ==================== Static Method Calls ====================

	/**
	 * Call a static method on a Haxe class (no arguments, no return)
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	bool CallStaticMethod(const FString& ClassName, const FString& MethodName);

	/**
	 * Call a static method with an integer argument
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value Integer argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	bool CallStaticMethodInt(const FString& ClassName, const FString& MethodName, int32 Value);

	/**
	 * Call a static method with a float argument
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value Float argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	bool CallStaticMethodFloat(const FString& ClassName, const FString& MethodName, float Value);

	/**
	 * Call a static method with a string argument
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param Value String argument
	 * @return True if call succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	bool CallStaticMethodString(const FString& ClassName, const FString& MethodName, const FString& Value);

	/**
	 * Call a static method and get an integer return value
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param DefaultValue Default value if call fails
	 * @return Return value from Haxe method, or DefaultValue on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	int32 CallStaticMethodReturnInt(const FString& ClassName, const FString& MethodName, int32 DefaultValue = 0);

	/**
	 * Call a static method and get a float return value
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @param DefaultValue Default value if call fails
	 * @return Return value from Haxe method, or DefaultValue on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	float CallStaticMethodReturnFloat(const FString& ClassName, const FString& MethodName, float DefaultValue = 0.0f);

	/**
	 * Call a static method and get a string return value
	 * @param ClassName Name of the Haxe class
	 * @param MethodName Name of the static method
	 * @return Return value from Haxe method, or empty string on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Call")
	FString CallStaticMethodReturnString(const FString& ClassName, const FString& MethodName);

	// ==================== Static Field Access ====================

	/**
	 * Get an integer static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param DefaultValue Default value if field not found
	 * @return Field value, or DefaultValue on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	int32 GetStaticInt(const FString& ClassName, const FString& FieldName, int32 DefaultValue = 0);

	/**
	 * Set an integer static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	bool SetStaticInt(const FString& ClassName, const FString& FieldName, int32 Value);

	/**
	 * Get a float static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param DefaultValue Default value if field not found
	 * @return Field value, or DefaultValue on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	float GetStaticFloat(const FString& ClassName, const FString& FieldName, float DefaultValue = 0.0f);

	/**
	 * Set a float static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	bool SetStaticFloat(const FString& ClassName, const FString& FieldName, float Value);

	/**
	 * Get a string static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @return Field value, or empty string on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	FString GetStaticString(const FString& ClassName, const FString& FieldName);

	/**
	 * Set a string static field
	 * @param ClassName Name of the Haxe class
	 * @param FieldName Name of the static field
	 * @param Value Value to set
	 * @return True if field was set successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Fields")
	bool SetStaticString(const FString& ClassName, const FString& FieldName, const FString& Value);

	// ==================== Utilities ====================

	/**
	 * Get the current .hl file path
	 * @return Path to the loaded .hl file
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Utilities")
	FString GetCurrentHLFilePath() const;

	/**
	 * Get the last error message from HLFFI
	 * @return Error message string
	 */
	UFUNCTION(BlueprintPure, Category = "HLFFI|Utilities")
	FString GetLastError() const;

	/**
	 * Force garbage collection in the HashLink VM
	 */
	UFUNCTION(BlueprintCallable, Category = "HLFFI|Utilities")
	void ForceGarbageCollection();

	// ==================== Delegates ====================

	/** Broadcast when VM starts successfully */
	UPROPERTY(BlueprintAssignable, Category = "HLFFI|Events")
	FOnHLFFIVMStarted OnVMStarted;

	/** Broadcast when VM stops */
	UPROPERTY(BlueprintAssignable, Category = "HLFFI|Events")
	FOnHLFFIVMStopped OnVMStopped;

	/** Broadcast when hot reload completes */
	UPROPERTY(BlueprintAssignable, Category = "HLFFI|Events")
	FOnHLFFIHotReload OnHotReload;

	// ==================== C++ API (for direct access) ====================

	/**
	 * Get the raw HLFFI VM handle (for advanced usage)
	 * @return Raw hlffi_vm pointer, or nullptr if not running
	 */
	hlffi_vm* GetVM() const { return VM; }

private:
	/** The HashLink VM instance */
	hlffi_vm* VM = nullptr;

	/** Path to the currently loaded .hl file */
	FString CurrentHLFilePath;

	/** Whether hot reload is enabled */
	bool bHotReloadEnabled = true;

	/** Whether the VM is in the process of initializing */
	bool bIsInitializing = false;

	/** Whether high-frequency timer processing is enabled */
	bool bHighFrequencyTimerEnabled = false;

	/** High-frequency timer update interval in ms */
	int32 TimerIntervalMs = 1;

	/** Handle for the high-frequency ticker (processes haxe.Timer events at 1ms) */
	FTSTicker::FDelegateHandle HighFrequencyTickerHandle;

	/** High-frequency ticker callback - processes TIMERS only */
	bool OnHighFrequencyTick(float DeltaTime);

	/** Internal helper to resolve .hl file path */
	FString ResolveHLFilePath(const FString& InPath) const;

	/** Internal helper for cleanup */
	void CleanupVM();

	/** Start the high-frequency ticker */
	void StartHighFrequencyTicker();

	/** Stop the high-frequency ticker */
	void StopHighFrequencyTicker();
};
