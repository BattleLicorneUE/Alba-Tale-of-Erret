
#include "DlgTextArgument.h"

#include "UObject/TextProperty.h"

#include "DlgConstants.h"
#include "DlgContext.h"
#include "DlgHelper.h"
#include "DlgDialogAttendee.h"
#include "NYReflectionHelper.h"
#include "Logging/DlgLogger.h"

FFormatArgumentValue FDlgTextArgument::ConstructFormatArgumentValue(const UDlgContext& Context, FName NodeOwner) const
{
	// If attendee name is not valid we use the node owner name
	const FName ValidAttendeeName = AttendeeName == NAME_None ? NodeOwner : AttendeeName;
	const UObject* Attendee = Context.GetAttendee(ValidAttendeeName);
	if (Attendee == nullptr)
	{
		FDlgLogger::Get().Errorf(
			TEXT("FAILED to construct text argument because the ATTENDEE is INVALID (Supplied Attendee = %s). \nContext:\n\t%s, DisplayString = %s, AttendeeName = %s, ArgumentType = %s"),
			*ValidAttendeeName.ToString(), *Context.GetContextString(), *DisplayString, *AttendeeName.ToString(), *ArgumentTypeToString(Type)
		);
		return FFormatArgumentValue(FText::FromString(TEXT("[CustomTextArgument is INVALID. Missing Attendee. Check log]")));
	}

	switch (Type)
	{
		case EDlgTextArgumentType::DialogueInt:
			return FFormatArgumentValue(IDlgDialogAttendee::Execute_GetIntValue(Attendee, VariableName));

		case EDlgTextArgumentType::ClassInt:
			return FFormatArgumentValue(FNYReflectionHelper::GetVariable<FNYIntProperty, int32>(Attendee, VariableName));

		case EDlgTextArgumentType::DialogueFloat:
			return FFormatArgumentValue(IDlgDialogAttendee::Execute_GetFloatValue(Attendee, VariableName));

		case EDlgTextArgumentType::ClassFloat:
			return FFormatArgumentValue(FNYReflectionHelper::GetVariable<FNYFloatProperty, float>(Attendee, VariableName));

		case EDlgTextArgumentType::ClassText:
			return FFormatArgumentValue(FNYReflectionHelper::GetVariable<FNYTextProperty, FText>(Attendee, VariableName));

		case EDlgTextArgumentType::DisplayName:
			return FFormatArgumentValue(IDlgDialogAttendee::Execute_GetAttendeeDisplayName(Attendee, NodeOwner));

		case EDlgTextArgumentType::Gender:
			return FFormatArgumentValue(IDlgDialogAttendee::Execute_GetAttendeeGender(Attendee));

		case EDlgTextArgumentType::Custom:
			if (CustomTextArgument == nullptr)
			{
				FDlgLogger::Get().Errorf(
					TEXT("Custom Text Argument is INVALID. Returning Error Text. Context:\n\t%s, Attendee = %s"),
					*Context.GetContextString(), Attendee ? *Attendee->GetPathName() : TEXT("INVALID")
				);
				return FFormatArgumentValue(FText::FromString(TEXT("[CustomTextArgument is INVALID. Missing Custom Text Argument. Check log]")));
			}

			return FFormatArgumentValue(CustomTextArgument->GetText(&Context, Attendee, DisplayString));

		default:
			checkNoEntry();
			return FFormatArgumentValue(0);
	}
}

void FDlgTextArgument::UpdateTextArgumentArray(const FText& Text, TArray<FDlgTextArgument>& InOutArgumentArray)
{
	TArray<FString> NewArgumentParams;
	FText::GetFormatPatternParameters(Text, NewArgumentParams);

	TArray<FDlgTextArgument> OldArguments = InOutArgumentArray;
	InOutArgumentArray.Empty();

	for (const FString& String : NewArgumentParams)
	{
		FDlgTextArgument Argument;
		Argument.DisplayString = String;

		// Replace with old argument values if display string matches
		for (const FDlgTextArgument& OldArgument : OldArguments)
		{
			if (String == OldArgument.DisplayString)
			{
				Argument = OldArgument;
				break;
			}
		}

		InOutArgumentArray.Add(Argument);
	}
}

FString FDlgTextArgument::ArgumentTypeToString(EDlgTextArgumentType Type)
{
	FString EnumValue;
	FDlgHelper::ConvertEnumToString<EDlgTextArgumentType>(TEXT("EDlgTextArgumentType"), Type, false, EnumValue);
	return EnumValue;
}
