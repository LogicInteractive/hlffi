# HLFFI Plugin for Unreal Engine 5.7

Integrate HashLink/Haxe scripting into your Unreal Engine project.

## Overview

The HLFFI Plugin embeds the HashLink Virtual Machine into Unreal Engine, allowing you to run Haxe code alongside your C++ and Blueprint logic. The plugin manages the VM lifecycle automatically - starting when you hit Play and stopping when you end the session.

## Features

- **Automatic VM Lifecycle**: VM starts/stops with Play sessions
- **Blueprint Integration**: Full Blueprint API for calling Haxe functions
- **Hot Reload**: Reload Haxe code without restarting the editor
- **Game Instance Subsystem**: Proper UE5 subsystem architecture
- **Two-Loop Architecture**: Separate event processing for MainLoop (frame rate) and Timers (high frequency)
- **High-Frequency Timers**: Optional 1ms timer precision for haxe.Timer

## Installation

### 1. Copy Plugin to Your Project

Copy the `HLFFIPlugin` folder to your project's `Plugins` directory:

```
YourProject/
└── Plugins/
    └── HLFFIPlugin/
```

### 2. Install HashLink Dependencies

You need the HashLink runtime library:

1. Download HashLink from [hashlink.haxe.org](https://hashlink.haxe.org/)
2. Copy `libhl.dll` to one of these locations:
   - `YourProject/Binaries/Win64/libhl.dll` (recommended)
   - `HLFFIPlugin/ThirdParty/hashlink/lib/Win64/libhl.dll`
3. Copy `libhl.lib` to:
   - `HLFFIPlugin/ThirdParty/hashlink/lib/Win64/libhl.lib`
4. Copy HashLink headers (`hl.h`, `hlmodule.h`) to:
   - `HLFFIPlugin/ThirdParty/hashlink/include/`

### 3. Regenerate Project Files

Right-click your `.uproject` file and select "Generate Visual Studio project files"

### 4. Build the Project

Build your project in Visual Studio or via UBT.

## Quick Start

### From Blueprint

1. Create or open a Level Blueprint
2. On **Begin Play**, add a "HLFFI Start VM" node
3. Set the `HLFilePath` to your compiled `.hl` file (relative to Content folder)
4. Call Haxe functions using "HLFFI Call Static Method" nodes

```
BeginPlay
    → HLFFI Start VM (HLFilePath: "Scripts/game.hl")
    → HLFFI Call Static Method (ClassName: "Game", MethodName: "start")
```

### From C++

```cpp
#include "HLFFISubsystem.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();

    // Get the subsystem
    UGameInstance* GI = GetGameInstance();
    UHLFFISubsystem* HLFFI = GI->GetSubsystem<UHLFFISubsystem>();

    // Start the VM
    HLFFI->StartVM(TEXT("Content/Scripts/game.hl"));

    // Call Haxe functions
    HLFFI->CallStaticMethod(TEXT("Game"), TEXT("start"));
}
```

## Haxe Side

Create your Haxe project and compile to `.hl` bytecode:

```haxe
// Game.hx
class Game
{
    public static function main():Void
    {
        trace("Hello from Haxe!");
    }

    public static function start():Void
    {
        trace("Game starting...");
    }

    public static function update(deltaTime:Float):Void
    {
        // Called every frame
    }
}
```

Compile with:
```bash
haxe -hl game.hl -main Game
```

## API Reference

### VM Lifecycle

| Function | Description |
|----------|-------------|
| `StartVM(Path)` | Start the HashLink VM with a .hl file |
| `StopVM()` | Stop the VM |
| `RestartVM(Path)` | Restart with a new .hl file |
| `IsVMRunning()` | Check if VM is active |

### Calling Haxe Methods

| Function | Description |
|----------|-------------|
| `CallStaticMethod(Class, Method)` | Call void method |
| `CallStaticMethodInt(Class, Method, Value)` | Call with int arg |
| `CallStaticMethodFloat(Class, Method, Value)` | Call with float arg |
| `CallStaticMethodString(Class, Method, Value)` | Call with string arg |
| `CallStaticMethodReturnInt(...)` | Call and get int return |
| `CallStaticMethodReturnFloat(...)` | Call and get float return |
| `CallStaticMethodReturnString(...)` | Call and get string return |

### Static Fields

| Function | Description |
|----------|-------------|
| `GetStaticInt(Class, Field)` | Read int field |
| `SetStaticInt(Class, Field, Value)` | Write int field |
| `GetStaticFloat(Class, Field)` | Read float field |
| `SetStaticFloat(Class, Field, Value)` | Write float field |
| `GetStaticString(Class, Field)` | Read string field |
| `SetStaticString(Class, Field, Value)` | Write string field |

### Hot Reload

| Function | Description |
|----------|-------------|
| `SetHotReloadEnabled(bool)` | Enable/disable hot reload |
| `TriggerHotReload()` | Manually trigger reload |

### High-Frequency Timers

| Function | Description |
|----------|-------------|
| `SetHighFrequencyTimerEnabled(bool, IntervalMs)` | Enable 1ms timer processing |
| `IsHighFrequencyTimerEnabled()` | Check if high-frequency mode is on |
| `GetHighFrequencyTimerInterval()` | Get current timer interval |

### Events (Delegates)

| Event | Description |
|-------|-------------|
| `OnVMStarted` | Broadcast when VM starts |
| `OnVMStopped` | Broadcast when VM stops |
| `OnHotReload` | Broadcast after hot reload (success/fail) |

## Event Loop Architecture

The plugin uses a two-loop architecture to handle Haxe events efficiently:

### MainLoop (Frame Rate)

Processed at frame rate (~60fps / ~16ms):
- `haxe.MainLoop.add()` callbacks
- Render and update logic

This is handled automatically in the subsystem's `Tick()` function.

### Timers (High Frequency)

Processed optionally at high frequency (~1ms):
- `haxe.Timer` events
- `haxe.Timer.delay()` callbacks

By default, timers are processed at frame rate (~16ms precision). For applications requiring precise timing (e.g., music games, precise animations), enable high-frequency mode:

```cpp
// From C++
HLFFI->SetHighFrequencyTimerEnabled(true, 1);  // 1ms intervals
```

```
// From Blueprint
HLFFI Set High Frequency Timer Enabled (Enable: true, IntervalMs: 1)
```

**Note:** High-frequency mode uses UE's FTSTicker which runs on the game thread. For true 1ms precision, your game must maintain a stable frame rate.

## Directory Structure

```
HLFFIPlugin/
├── HLFFIPlugin.uplugin         # Plugin descriptor
├── README.md                   # This file
├── Source/
│   └── HLFFIPlugin/
│       ├── HLFFIPlugin.Build.cs
│       ├── Private/
│       │   ├── HLFFIPluginModule.cpp
│       │   ├── HLFFISubsystem.cpp
│       │   └── HLFFIBlueprintLibrary.cpp
│       └── Public/
│           ├── HLFFIPluginModule.h
│           ├── HLFFISubsystem.h
│           └── HLFFIBlueprintLibrary.h
└── ThirdParty/
    ├── hlffi/
    │   └── hlffi.h             # HLFFI header
    └── hashlink/
        ├── include/            # HashLink headers (hl.h, hlmodule.h)
        └── lib/
            └── Win64/          # libhl.lib, libhl.dll
```

## Troubleshooting

### "libhl.dll not found"

Make sure `libhl.dll` is either:
- In your project's `Binaries/Win64/` folder
- In the plugin's `ThirdParty/hashlink/lib/Win64/` folder

### "Failed to load bytecode"

- Verify the .hl file path is correct
- Check that the .hl file was compiled with a compatible HashLink version
- Recompile your Haxe code if you updated HashLink

### "VM not initialized"

Make sure `StartVM()` was called before trying to call Haxe functions.

## Requirements

- Unreal Engine 5.7+
- Windows 64-bit
- Visual Studio 2022
- HashLink 1.12+ (for hot reload support)
- Haxe 4.x (for compiling .hl files)

## License

MIT License - Same as HLFFI and HashLink

## Links

- [HLFFI Repository](https://github.com/LogicInteractive/hlffi)
- [HashLink](https://hashlink.haxe.org/)
- [Haxe](https://haxe.org/)
