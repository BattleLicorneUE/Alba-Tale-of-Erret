
#include "DlgEvent.h"

#include "DlgConstants.h"
#include "DlgContext.h"
#include "NYReflectionHelper.h"
#include "DlgDialogAttendee.h"
#include "DlgHelper.h"
#include "Logging/DlgLogger.h"

void FDlgEvent::Call(UDlgContext& Context, const FString& ContextString, UObject* Attendee) const
{
	const bool bHasAttendee = ValidateIsAttendeeValid(
		Context,
		FString::Printf(TEXT("%s::Call"), *ContextString),
		Attendee
	);

	// We don't care if it has a attendee, but warn nonetheless by calling validate it before this
	if (EventType == EDlgEventType::Custom)
	{
		if (CustomEvent == nullptr)
		{
			FDlgLogger::Get().Warningf(
				TEXT("Custom Event is empty (not valid). Ignoring. Context:\n\t%s, Attendee = %s"),
				*Context.GetContextString(), Attendee ? *Attendee->GetPathName() : TEXT("INVALID")
			);
			return;
		}

		CustomEvent->EnterEvent(&Context, Attendee);
		return;
	}

	// Must have attendee from this point onwards
	if (MustHaveAttendee() && !bHasAttendee)
	{
		return;
	}
	switch (EventType)
	{
		case EDlgEventType::Event:
			IDlgDialogAttendee::Execute_OnDialogueEvent(Attendee, &Context, EventName);
			break;

		case EDlgEventType::ModifyInt:
			IDlgDialogAttendee::Execute_ModifyIntValue(Attendee, EventName, bDelta, IntValue);
			break;
		case EDlgEventType::ModifyFloat:
			IDlgDialogAttendee::Execute_ModifyFloatValue(Attendee, EventName, bDelta, FloatValue);
			break;
		case EDlgEventType::ModifyBool:
			IDlgDialogAttendee::Execute_ModifyBoolValue(Attendee, EventName, bValue);
			break;
		case EDlgEventType::ModifyName:
			IDlgDialogAttendee::Execute_ModifyNameValue(Attendee, EventName, NameValue);
			break;

		case EDlgEventType::ModifyClassIntVariable:
			FNYReflectionHelper::ModifyVariable<FNYIntProperty>(Attendee, EventName, IntValue, bDelta);
			break;
		case EDlgEventType::ModifyClassFloatVariable:
			FNYReflectionHelper::ModifyVariable<FNYFloatProperty>(Attendee, EventName, FloatValue, bDelta);
			break;
		case EDlgEventType::ModifyClassBoolVariable:
			FNYReflectionHelper::SetVariable<FNYBoolProperty>(Attendee, EventName, bValue);
			break;
		case EDlgEventType::ModifyClassNameVariable:
			FNYReflectionHelper::SetVariable<FNYNameProperty>(Attendee, EventName, NameValue);
			break;

		default:
			checkNoEntry();
	}
}

bool FDlgEvent::ValidateIsAttendeeValid(const UDlgContext& Context, const FString& ContextString, const UObject* Attendee) const
{
	if (IsValid(Attendee))
	{
		return true;
	}

	if (MustHaveAttendee())
	{
		FDlgLogger::Get().Errorf(
			TEXT("%s - Event FAILED because the PARTICIPANT is INVALID. \nContext:\n\t%s, \n\tAttendeeName = %s, EventType = %s, EventName = %s, CustomEvent = %s"),
			*ContextString, *Context.GetContextString(), *AttendeeName.ToString(), *EventTypeToString(EventType), *EventName.ToString(), *GetCustomEventName()
		);
	}
	else
	{
		FDlgLogger::Get().Warningf(
			TEXT("%s - Event WARNING because the PARTICIPANT is INVALID. The call will NOT FAIL, but the attendee is not present. \nContext:\n\t%s, \n\tAttendeeName = %s, EventType = %s, EventName = %s, CustomEvent = %s"),
			*ContextString, *Context.GetContextString(), *AttendeeName.ToString(), *EventTypeToString(EventType), *EventName.ToString(), *GetCustomEventName()
		);
	}

	return false;
}

FString FDlgEvent::EventTypeToString(EDlgEventType Type)
{
	FString EnumValue;
	FDlgHelper::ConvertEnumToString<EDlgEventType>(TEXT("EDlgEventType"), Type, false, EnumValue);
	return EnumValue;
}
