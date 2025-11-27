#include "UEHL_FFI.h"
#include <hlMain.h>

//FColor IntToFColor(int ColorValue)
//{
//    FColor Color;
//    Color.R = (ColorValue >> 24) & 0xFF;
//    Color.G = (ColorValue >> 16) & 0xFF;
//    Color.B = (ColorValue >> 8) & 0xFF;
//    Color.A = ColorValue & 0xFF;
//    return Color;
//}

#define UINT_TO_FCOLOR(color) FColor(\
    ((color) >> 16) & 0xFF, /* Red   */\
    ((color) >> 8) & 0xFF,  /* Green */\
    (color) & 0xFF,         /* Blue  */\
    ((color) >> 24) & 0xFF  /* Alpha */\
)

HL_PRIM void HL_NAME(hx_ue_log)(const char* msg)
{
 //   AsyncTask(ENamedThreads::GameThread, [msg]()
 //   {
        FString str(msg);
        UE_LOG(LogTemp, Warning, TEXT("%s"), *str);
//    });*/
}
DEFINE_PRIM(_VOID, hx_ue_log, _BYTES);

HL_PRIM void HL_NAME(hx_ue_AddOnScreenDebugMessage)(const char* DebugMessage, double TimeToDisplay, int Color, int Key)
//HL_PRIM void HL_NAME(hx_ue_AddOnScreenDebugMessage)(const char* DebugMessage)
{
    //AsyncTask(ENamedThreads::GameThread, [=]()
    //{
        FString str = ANSI_TO_TCHAR(DebugMessage);
        if (GEngine)
        //GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, *str);
        GEngine->AddOnScreenDebugMessage(Key, TimeToDisplay, UINT_TO_FCOLOR(Color), FString(DebugMessage));
    //});
}
//DEFINE_PRIM(_VOID, hx_ue_AddOnScreenDebugMessage, _BYTES);
DEFINE_PRIM(_VOID, hx_ue_AddOnScreenDebugMessage, _BYTES _F64 _I32 _I32);

HL_PRIM double HL_NAME(hx_ue_GetTimeSeconds)()
{
    UWorld* World = GEngine->GetWorldContexts()[0].World();
    if (World)
    {
        double now = World->GetTimeSeconds();
        return now;
    }
    else
        return 0;
}
DEFINE_PRIM(_F64, hx_ue_GetTimeSeconds, _NO_ARG);



//Key	A unique key to prevent the same message from being added multiple times.
//TimeToDisplay	How long to display the message, in seconds.
//DisplayColor	The color to display the text in.
//DebugMessage	The message to display.