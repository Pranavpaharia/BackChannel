// Copyright 2017 Andrew Grant
// Unless explicitly stated otherwise all files in this repository 
// are licensed under BSD License 2.0. All Rights Reserved.

#include "BackChannel/Transport/IBackChannelTransport.h"
#include "BackChannel/Protocol/OSC/BackChannelOSC.h"
#include "EngineMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class FBackChannelTestOSCBase : public FAutomationTestBase
{

public:

	FBackChannelTestOSCBase(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask) {}

};

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FBackChannelTestOSCMessage, FBackChannelTestOSCBase, "BackChannel.TestOSCMessage", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FBackChannelTestOSCBundle, FBackChannelTestOSCBase, "BackChannel.TestOSCBundle", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FBackChannelTestOSCMessage::RunTest(const FString& Parameters)
{
	FBackChannelOSCMessage Message(OSCPacketMode::Write);
	
	Message.SetAddress(TEXT("/foo"));
	
	int32 IntValue = 1;
	float FloatValue = 2.5;
	FString StringValue = TEXT("Hello!");
	TArray<int8> AnswerArray;

	const int kArraySize = 33;
	const int kArrayValue = 42;

	for (int i = 0; i < kArraySize; i++)
	{
		AnswerArray.Add(kArrayValue);
	}

	Message << IntValue << FloatValue << StringValue << AnswerArray;

	FString Address = Message.GetAddress();
	FString Tags = Message.GetTags();
	const int32 ArgSize = Message.GetArgumentSize();

	const int RoundedStringSize = FBackChannelOSCMessage::RoundedArgumentSize(StringValue.Len() + 1);
	const int RoundedArraySize = FBackChannelOSCMessage::RoundedArgumentSize(kArraySize);

	const int ExpectedArgSize = 4 + 4 + RoundedStringSize + RoundedArraySize;
	const int ExpectedBufferSize = ExpectedArgSize + FBackChannelOSCMessage::RoundedArgumentSize(Address.Len() + 1) + FBackChannelOSCMessage::RoundedArgumentSize(Tags.Len() + 1);

	// verify this address and tags...
	check(Address == TEXT("/foo"));
	check(Tags == TEXT("ifsb"));
	check(ArgSize == ExpectedArgSize);

	TArray<uint8> Buffer;

	Message.WriteToBuffer(Buffer);

	check(Buffer.Num() == ExpectedBufferSize);
	check(FBackChannelOSCPacket::GetType(Buffer.GetData(), Buffer.Num()) == OSCPacketType::Message);

	TSharedPtr<FBackChannelOSCMessage> NewMessage = FBackChannelOSCMessage::CreateFromBuffer(Buffer.GetData(), Buffer.Num());

	// read them back
	int32 OutIntValue(0);
	float OutFloatValue(0);
	FString OutStringValue;
	TArray<uint8> OutArray;

	OutArray.AddUninitialized(kArraySize);

	*NewMessage << OutIntValue << OutFloatValue << OutStringValue << OutArray;

	check(OutIntValue == IntValue);
	check(OutFloatValue == OutFloatValue);
	check(OutStringValue == OutStringValue);
	
	for (int i = 0; i < OutArray.Num(); i++)
	{
		check(OutArray[i] == kArrayValue);
	}

	return true;
}


bool FBackChannelTestOSCBundle::RunTest(const FString& Parameters)
{
	TSharedPtr<FBackChannelOSCBundle> Bundle = MakeShareable(new FBackChannelOSCBundle(OSCPacketMode::Write));

	FString TestString1 = TEXT("Hello World!");
	FString TestString2 = TEXT("Hello World Again!");

	Bundle->AddElement(*TestString1, (TestString1.Len() + 1) * sizeof(TCHAR));
	Bundle->AddElement(*TestString2, (TestString2.Len() + 1) * sizeof(TCHAR));

	// first loop tests the bundle we just constructed, second loop tests it
	// after serializing to and from a buffer
	for (int i = 0; i < 2; i++)
	{
		check(Bundle->GetElementCount() == 2);

		TArray<uint8> Data1, Data2;
		Data1 = Bundle->GetElement(0);
		Data2 = Bundle->GetElement(1);

		const TCHAR* pString1 = (const TCHAR*)Data1.GetData();
		const TCHAR* pString2 = (const TCHAR*)Data2.GetData();

		check(TestString1 == pString1);
		check(TestString2 == pString2);

		TArray<uint8> BundleData;
		
		// write to the buffer
		Bundle->WriteToBuffer(BundleData);

		TSharedPtr<FBackChannelOSCPacket> Packet = FBackChannelOSCPacket::CreateFromBuffer(BundleData.GetData(), BundleData.Num());

		check(Packet->GetType() == OSCPacketType::Bundle);

		Bundle = StaticCastSharedPtr<FBackChannelOSCBundle>(Packet);
	}

	return true;
}



#endif // WITH_DEV_AUTOMATION_TESTS