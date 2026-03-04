#pragma once

namespace  DebugUtils
{
	static void Print(const FString& Msg,bool bOutputScreen = true, const FColor& Color = FColor::MakeRandomColor(), int32 Inkey = -1)
	{
		//디버깅 로그
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		
		if (GEngine && bOutputScreen)
		{
			//스크린 로그
			GEngine->AddOnScreenDebugMessage(Inkey, 7.0f, Color, Msg);
		}
	}
	
	static void PrintOnScreen(const FString& Msg, int32 Inkey = -1, float TimeToDisplay = 5.0f, const FColor& Color = FColor::Green)
	{
		if (GEngine)
		{
			//스크린 로그
			GEngine->AddOnScreenDebugMessage(Inkey, TimeToDisplay, Color, Msg);
		}
	}
}
