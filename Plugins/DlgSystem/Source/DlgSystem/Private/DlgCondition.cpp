
#include "DlgCondition.h"

#include "DlgConstants.h"
#include "DlgMemory.h"
#include "DlgContext.h"
#include "Nodes/DlgNode.h"
#include "NYReflectionHelper.h"
#include "Kismet/GameplayStatics.h"
#include "DlgDialogAttendee.h"
#include "DlgHelper.h"
#include "Logging/DlgLogger.h"

bool FDlgCondition::EvaluateArray(const UDlgContext& Context, const TArray<FDlgCondition>& ConditionsArray, FName DefaultAttendeeName)
{
	bool bHasAnyWeak = false;
	bool bHasSuccessfulWeak = false;

	for (const FDlgCondition& Condition : ConditionsArray)
	{
		const FName AttendeeName = Condition.AttendeeName == NAME_None ? DefaultAttendeeName : Condition.AttendeeName;
		const bool bSatisfied = Condition.IsConditionMet(Context, Context.GetAttendee(AttendeeName));
		if (Condition.Strength == EDlgConditionStrength::Weak)
		{
			bHasAnyWeak = true;
			bHasSuccessfulWeak = bHasSuccessfulWeak || bSatisfied;
		}
		else if (!bSatisfied)
		{
			// All must be satisfied
			return false;
		}
	}

	return bHasSuccessfulWeak || !bHasAnyWeak;
}

bool FDlgCondition::IsConditionMet(const UDlgContext& Context, const UObject* Attendee) const
{
	bool bHasAttendee = true;
	if (IsAttendeeInvolved())
	{
		bHasAttendee = ValidateIsAttendeeValid(Context, TEXT("IsConditionMet"), Attendee);
	}

	// We don't care if it has a attendee, but warn nonetheless by calling validate it before this
	if (ConditionType == EDlgConditionType::Custom)
	{
		if (CustomCondition == nullptr)
		{
			FDlgLogger::Get().Errorf(
				TEXT("Custom Condition is empty (not valid). IsConditionMet returning false.\nContext:\n\t%s, Attendee = %s"),
				*Context.GetContextString(), Attendee ? *Attendee->GetPathName() : TEXT("INVALID")
			);
			return false;
		}

		return CustomCondition->IsConditionMet(&Context, Attendee);
	}

	// Must have attendee from this point onwards
	if (!bHasAttendee)
	{
		return false;
	}
	switch (ConditionType)
	{
		case EDlgConditionType::EventCall:
			return IDlgDialogAttendee::Execute_CheckCondition(Attendee, &Context, CallbackName) == bBoolValue;

		case EDlgConditionType::BoolCall:
			return CheckBool(Context, IDlgDialogAttendee::Execute_GetBoolValue(Attendee, CallbackName));

		case EDlgConditionType::FloatCall:
			return CheckFloat(Context, IDlgDialogAttendee::Execute_GetFloatValue(Attendee, CallbackName));

		case EDlgConditionType::IntCall:
			return CheckInt(Context, IDlgDialogAttendee::Execute_GetIntValue(Attendee, CallbackName));

		case EDlgConditionType::NameCall:
			return CheckName(Context, IDlgDialogAttendee::Execute_GetNameValue(Attendee, CallbackName));


		case EDlgConditionType::ClassBoolVariable:
			return CheckBool(Context, FNYReflectionHelper::GetVariable<FNYBoolProperty, bool>(Attendee, CallbackName));

		case EDlgConditionType::ClassFloatVariable:
			return CheckFloat(Context, FNYReflectionHelper::GetVariable<FNYFloatProperty, float>(Attendee, CallbackName));

		case EDlgConditionType::ClassIntVariable:
			return CheckInt(Context, FNYReflectionHelper::GetVariable<FNYIntProperty, int32>(Attendee, CallbackName));

		case EDlgConditionType::ClassNameVariable:
			return CheckName(Context, FNYReflectionHelper::GetVariable<FNYNameProperty, FName>(Attendee, CallbackName));


		case EDlgConditionType::WasNodeVisited:
			if (bLongTermMemory)
			{
				return FDlgMemory::Get().IsNodeVisited(Context.GetDialogueGUID(), IntValue, GUID) == bBoolValue;
			}

			return Context.GetHistoryOfThisContext().Contains(IntValue, GUID) == bBoolValue;

		case EDlgConditionType::HasSatisfiedChild:
			{
				// Use the GUID if it is valid as it is more reliable
				const UDlgNode* Node = GUID.IsValid() ? Context.GetNodeFromGUID(GUID) : Context.GetNodeFromIndex(IntValue);
				return Node != nullptr ? Node->HasAnySatisfiedChild(Context, {}) == bBoolValue : false;
			}

		default:
			checkNoEntry();
			return false;
	}
}

bool FDlgCondition::CheckFloat(const UDlgContext& Context, float Value) const
{
	float ValueToCheckAgainst = FloatValue;
	if (CompareType == EDlgCompare::ToVariable || CompareType == EDlgCompare::ToClassVariable)
	{
		const UObject* OtherAttendee = Context.GetAttendee(OtherAttendeeName);
		if (!ValidateIsAttendeeValid(Context, TEXT("CheckFloat"), OtherAttendee))
		{
			return false;
		}

		if (CompareType == EDlgCompare::ToVariable)
		{
			ValueToCheckAgainst = IDlgDialogAttendee::Execute_GetFloatValue(OtherAttendee, OtherVariableName);
		}
		else
		{
			ValueToCheckAgainst = FNYReflectionHelper::GetVariable<FNYFloatProperty, float>(OtherAttendee, OtherVariableName);
		}
	}

	switch (Operation)
	{
		case EDlgOperation::Equal:
			return FMath::IsNearlyEqual(Value, ValueToCheckAgainst);

		case EDlgOperation::Greater:
			return Value > ValueToCheckAgainst;

		case EDlgOperation::GreaterOrEqual:
			return Value >= ValueToCheckAgainst;

		case EDlgOperation::Less:
			return Value < ValueToCheckAgainst;

		case EDlgOperation::LessOrEqual:
			return Value <= ValueToCheckAgainst;

		case EDlgOperation::NotEqual:
			return !FMath::IsNearlyEqual(Value, ValueToCheckAgainst);

		default:
			FDlgLogger::Get().Errorf(
				TEXT("Invalid Operation in float based condition.\nContext:\n\t%s"),
				*Context.GetContextString()
			);
			return false;
	}
}

bool FDlgCondition::CheckInt(const UDlgContext& Context, int32 Value) const
{
	int32 ValueToCheckAgainst = IntValue;
	if (CompareType == EDlgCompare::ToVariable || CompareType == EDlgCompare::ToClassVariable)
	{
		const UObject* OtherAttendee = Context.GetAttendee(OtherAttendeeName);
		if (!ValidateIsAttendeeValid(Context, TEXT("CheckInt"), OtherAttendee))
		{
			return false;
		}

		if (CompareType == EDlgCompare::ToVariable)
		{
			ValueToCheckAgainst = IDlgDialogAttendee::Execute_GetIntValue(OtherAttendee, OtherVariableName);
		}
		else
		{
			ValueToCheckAgainst = FNYReflectionHelper::GetVariable<FNYIntProperty, int32>(OtherAttendee, OtherVariableName);
		}
	}

	switch (Operation)
	{
		case EDlgOperation::Equal:
			return Value == ValueToCheckAgainst;

		case EDlgOperation::Greater:
			return Value > ValueToCheckAgainst;

		case EDlgOperation::GreaterOrEqual:
			return Value >= ValueToCheckAgainst;

		case EDlgOperation::Less:
			return Value < ValueToCheckAgainst;

		case EDlgOperation::LessOrEqual:
			return Value <= ValueToCheckAgainst;

		case EDlgOperation::NotEqual:
			return Value != ValueToCheckAgainst;

		default:
			FDlgLogger::Get().Errorf(
				TEXT("Invalid Operation in int based condition.\nContext:\n\t%s"),
				*Context.GetContextString()
			);
			return false;
	}
}

bool FDlgCondition::CheckBool(const UDlgContext& Context, bool bValue) const
{
	bool bResult = bValue;
	if (CompareType == EDlgCompare::ToVariable || CompareType == EDlgCompare::ToClassVariable)
	{
		const UObject* OtherAttendee = Context.GetAttendee(OtherAttendeeName);
		if (!ValidateIsAttendeeValid(Context, TEXT("CheckBool"), OtherAttendee))
		{
			return false;
		}

		bool bValueToCheckAgainst;
		if (CompareType == EDlgCompare::ToVariable)
		{
			bValueToCheckAgainst = IDlgDialogAttendee::Execute_GetBoolValue(OtherAttendee, OtherVariableName);
		}
		else
		{
			bValueToCheckAgainst = FNYReflectionHelper::GetVariable<FNYBoolProperty, bool>(OtherAttendee, OtherVariableName);
		}

		// Check if value matches other variable
		bResult = bValue == bValueToCheckAgainst;
	}

	return bResult == bBoolValue;
}

bool FDlgCondition::CheckName(const UDlgContext& Context, FName Value) const
{
	FName ValueToCheckAgainst = NameValue;
	if (CompareType == EDlgCompare::ToVariable || CompareType == EDlgCompare::ToClassVariable)
	{
		const UObject* OtherAttendee = Context.GetAttendee(OtherAttendeeName);
		if (!ValidateIsAttendeeValid(Context, TEXT("CheckName"), OtherAttendee))
		{
			return false;
		}

		if (CompareType == EDlgCompare::ToVariable)
		{
			ValueToCheckAgainst = IDlgDialogAttendee::Execute_GetNameValue(OtherAttendee, OtherVariableName);
		}
		else
		{
			ValueToCheckAgainst = FNYReflectionHelper::GetVariable<FNYNameProperty, FName>(OtherAttendee, OtherVariableName);
		}
	}

	const bool bResult = ValueToCheckAgainst == Value;
	return bResult == bBoolValue;
}

bool FDlgCondition::ValidateIsAttendeeValid(const UDlgContext& Context, const FString& ContextString, const UObject* Attendee) const
{
	if (IsValid(Attendee))
	{
		return true;
	}

	FDlgLogger::Get().Errorf(
		TEXT("%s FAILED because the PARTICIPANT is INVALID.\nContext:\n\t%s, ConditionType = %s"),
		*ContextString, *Context.GetContextString(), *ConditionTypeToString(ConditionType)
	);
	return false;
}

bool FDlgCondition::IsAttendeeInvolved() const
{
	return ConditionType != EDlgConditionType::HasSatisfiedChild
		&& ConditionType != EDlgConditionType::WasNodeVisited;
}

bool FDlgCondition::IsSecondAttendeeInvolved() const
{
	// Second attendee requires first attendee
	return CompareType != EDlgCompare::ToConst && IsAttendeeInvolved();
}

FString FDlgCondition::ConditionTypeToString(EDlgConditionType Type)
{
	FString EnumValue;
	if (FDlgHelper::ConvertEnumToString<EDlgConditionType>(TEXT("EDlgConditionType"), Type, false, EnumValue))
		return EnumValue;

	return EnumValue;
}
