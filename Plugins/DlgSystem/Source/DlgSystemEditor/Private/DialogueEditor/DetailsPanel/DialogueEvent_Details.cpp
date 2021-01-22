
#include "DialogueEvent_Details.h"

#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "IDetailChildrenBuilder.h"

#include "Nodes/DlgNode.h"
#include "NYReflectionHelper.h"
#include "DialogueDetailsPanelUtils.h"
#include "DialogueEditor/Nodes/DialogueGraphNode.h"
#include "Widgets/SDialogueTextPropertyPickList.h"
#include "Widgets/DialogueTextPropertyPickList_CustomRowHelper.h"
#include "DlgHelper.h"
#include "Widgets/DialogueEnumTypeWithObject_CustomRowHelper.h"
#include "Widgets/DialogueObject_CustomRowHelper.h"

#define LOCTEXT_NAMESPACE "DialogueEvent_Details"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDialogueCustomEventization
void FDialogueEvent_Details::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	Dialogue = FDialogueDetailsPanelUtils::GetDialogueFromPropertyHandle(StructPropertyHandle.ToSharedRef());
	PropertyUtils = StructCustomizationUtils.GetPropertyUtilities();

	// Cache the Property Handle for the EventType
	AttendeeNamePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, AttendeeName));
	EventTypePropertyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, EventType));
	check(AttendeeNamePropertyHandle.IsValid());
	check(EventTypePropertyHandle.IsValid());

	// Register handler for event type change
	EventTypePropertyHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &Self::OnEventTypeChanged, true)
	);

	const bool bShowOnlyInnerProperties = StructPropertyHandle->GetProperty()->HasMetaData(META_ShowOnlyInnerProperties);
	if (!bShowOnlyInnerProperties)
	{
		HeaderRow.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			];
	}
}

void FDialogueEvent_Details::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const bool bHasDialogue = Dialogue != nullptr;


	// Common AttendeeName
	{
		FDetailWidgetRow* DetailWidgetRow = &StructBuilder.AddCustomRow(LOCTEXT("AttendeeNameSearchKey", "Attendee Name"));

		AttendeeNamePropertyRow = MakeShared<FDialogueTextPropertyPickList_CustomRowHelper>(DetailWidgetRow, AttendeeNamePropertyHandle);
		AttendeeNamePropertyRow->SetTextPropertyPickListWidget(
			SNew(SDialogueTextPropertyPickList)
			.AvailableSuggestions(this, &Self::GetAllDialoguesAttendeeNames)
			.OnTextCommitted(this, &Self::HandleTextCommitted)
			.HasContextCheckbox(bHasDialogue)
			.IsContextCheckBoxChecked(true)
			.CurrentContextAvailableSuggestions(this, &Self::GetCurrentDialogueAttendeeNames)
		)
		.Update();
	}

	// EventType
	{
		EventTypePropertyRow = &StructBuilder.AddProperty(EventTypePropertyHandle.ToSharedRef());

		// Add Custom buttons
		EventTypePropertyRow_CustomDisplay = MakeShared<FDialogueEnumTypeWithObject_CustomRowHelper>(
            EventTypePropertyRow,
            Dialogue,
            AttendeeNamePropertyHandle
        );
		EventTypePropertyRow_CustomDisplay->SetEnumType(EDialogueEnumWithObjectType::Event);
		EventTypePropertyRow_CustomDisplay->Update();
	}

	// EventName
	{
		const TSharedPtr<IPropertyHandle> EventNamePropertyHandle = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FDlgEvent, EventName)
		);
		FDetailWidgetRow* DetailWidgetRow = &StructBuilder.AddCustomRow(LOCTEXT("EventNameSearchKey", "Event Name"));

		EventNamePropertyRow = MakeShared<FDialogueTextPropertyPickList_CustomRowHelper>(DetailWidgetRow, EventNamePropertyHandle);
		EventNamePropertyRow->SetTextPropertyPickListWidget(
				SNew(SDialogueTextPropertyPickList)
				.AvailableSuggestions(this, &Self::GetAllDialoguesEventNames)
				.OnTextCommitted(this, &Self::HandleTextCommitted)
				.HasContextCheckbox(bHasDialogue)
				.IsContextCheckBoxChecked(false)
				.CurrentContextAvailableSuggestions(this, &Self::GetCurrentDialogueEventNames)
		)
		.SetVisibility(CREATE_VISIBILITY_CALLBACK(&Self::GetEventNameVisibility))
		.Update();
	}

	// IntValue
	{
		IntValuePropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, IntValue)).ToSharedRef()
		);
		IntValuePropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetIntValueVisibility));
	}

	// FloatValue
	{
		FloatValuePropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, FloatValue)).ToSharedRef()
		);
		FloatValuePropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetFloatValueVisibility));
	}

	// NameValue
	{
		NameValuePropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, NameValue)).ToSharedRef()
		);
		NameValuePropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetNameValueVisibility));
	}

	// bDelta
	{
		BoolDeltaPropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, bDelta)).ToSharedRef()
		);
		BoolDeltaPropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetBoolDeltaVisibility));
	}

	// bValue
	{
		BoolValuePropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, bValue)).ToSharedRef()
		);
		BoolValuePropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetBoolValueVisibility));
	}

	// CustomEvent
	{
		CustomEventPropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDlgEvent, CustomEvent)).ToSharedRef()
		);
		CustomEventPropertyRow->Visibility(CREATE_VISIBILITY_CALLBACK(&Self::GetCustomEventVisibility));

		// Add Custom buttons
		CustomEventPropertyRow_CustomDisplay = MakeShared<FDialogueObject_CustomRowHelper>(CustomEventPropertyRow);
		CustomEventPropertyRow_CustomDisplay->Update();
		CustomEventPropertyRow_CustomDisplay->SetFunctionNameToOpen(
			EDialogueBlueprintOpenType::Event,
			GET_FUNCTION_NAME_CHECKED(UDlgEventCustom, EnterEvent)
		);
	}

	// Cache the initial event type
	OnEventTypeChanged(false);
}

void FDialogueEvent_Details::OnEventTypeChanged(bool bForceRefresh)
{
	// Update to the new type
	uint8 Value = 0;
	if (EventTypePropertyHandle->GetValue(Value) != FPropertyAccess::Success)
	{
		return;
	}
	EventType = static_cast<EDlgEventType>(Value);

	// Update the display name/tooltips
	FText EventNameDisplayName = LOCTEXT("EventNameDisplayName", "Variable Name");
	FText EventNameToolTip = LOCTEXT("EventNameToolTip", "Name of the relevant variable");
	if (EventType == EDlgEventType::Event)
	{
		EventNameDisplayName = LOCTEXT("DlgEvent_EventNameDisplayName", "Event Name");
		EventNameToolTip = LOCTEXT("DlgEvent_EventNameToolTip", "Name of the relevant event");
	}

	EventNamePropertyRow->SetDisplayName(EventNameDisplayName)
		.SetToolTip(EventNameToolTip)
		.Update();

	// Refresh the view, without this some names/tooltips won't get refreshed
	if (bForceRefresh && PropertyUtils.IsValid())
	{
		PropertyUtils->ForceRefresh();
	}
}

TArray<FName> FDialogueEvent_Details::GetAllDialoguesEventNames() const
{
	TArray<FName> Suggestions;
	const FName AttendeeName = FDialogueDetailsPanelUtils::GetAttendeeNameFromPropertyHandle(AttendeeNamePropertyHandle.ToSharedRef());

	switch (EventType)
	{
	case EDlgEventType::ModifyBool:
		UDlgManager::GetAllDialoguesBoolNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyFloat:
		UDlgManager::GetAllDialoguesFloatNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyInt:
		UDlgManager::GetAllDialoguesIntNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyName:
		UDlgManager::GetAllDialoguesNameNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyClassIntVariable:
		if (Dialogue)
		{
			FNYReflectionHelper::GetVariableNames(
				Dialogue->GetAttendeeClass(AttendeeName),
				FNYIntProperty::StaticClass(),
				Suggestions,
				GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
			);
			FDlgHelper::SortDefault(Suggestions);
		}
		break;

	case EDlgEventType::ModifyClassFloatVariable:
		if (Dialogue)
		{
			FNYReflectionHelper::GetVariableNames(
				Dialogue->GetAttendeeClass(AttendeeName),
				FNYFloatProperty::StaticClass(),
				Suggestions,
				GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
			);
			FDlgHelper::SortDefault(Suggestions);
		}
		break;

	case EDlgEventType::ModifyClassBoolVariable:
		if (Dialogue)
		{
			FNYReflectionHelper::GetVariableNames(
				Dialogue->GetAttendeeClass(AttendeeName),
				FNYBoolProperty::StaticClass(),
				Suggestions,
				GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
			);
			FDlgHelper::SortDefault(Suggestions);
		}
		break;

	case EDlgEventType::ModifyClassNameVariable:
		if (Dialogue)
		{
			FNYReflectionHelper::GetVariableNames(
				Dialogue->GetAttendeeClass(AttendeeName),
				FNYNameProperty::StaticClass(),
				Suggestions,
				GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
			);
			FDlgHelper::SortDefault(Suggestions);
		}
		break;


	case EDlgEventType::Event:
	default:
		UDlgManager::GetAllDialoguesEventNames(AttendeeName, Suggestions);
		break;
	}

	return Suggestions;
}

TArray<FName> FDialogueEvent_Details::GetCurrentDialogueEventNames() const
{
	if (Dialogue == nullptr)
	{
		return {};
	}

	const FName AttendeeName = FDialogueDetailsPanelUtils::GetAttendeeNameFromPropertyHandle(AttendeeNamePropertyHandle.ToSharedRef());
	TSet<FName> Suggestions;

	switch (EventType)
	{
	case EDlgEventType::ModifyBool:
		Dialogue->GetBoolNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyName:
		Dialogue->GetNameNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyFloat:
		Dialogue->GetFloatNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyInt:
		Dialogue->GetIntNames(AttendeeName, Suggestions);
		break;

	case EDlgEventType::ModifyClassIntVariable:
		FNYReflectionHelper::GetVariableNames(
			Dialogue->GetAttendeeClass(AttendeeName),
			FNYIntProperty::StaticClass(),
			Suggestions,
			GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
		);
		break;

	case EDlgEventType::ModifyClassFloatVariable:
		FNYReflectionHelper::GetVariableNames(
			Dialogue->GetAttendeeClass(AttendeeName),
			FNYFloatProperty::StaticClass(),
			Suggestions,
			GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
		);
		break;

	case EDlgEventType::ModifyClassBoolVariable:
		FNYReflectionHelper::GetVariableNames(
			Dialogue->GetAttendeeClass(AttendeeName),
			FNYBoolProperty::StaticClass(),
			Suggestions,
			GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
		);
		break;

	case EDlgEventType::ModifyClassNameVariable:
		FNYReflectionHelper::GetVariableNames(
			Dialogue->GetAttendeeClass(AttendeeName),
			FNYNameProperty::StaticClass(),
			Suggestions,
			GetDefault<UDlgSystemSettings>()->BlacklistedReflectionClasses
		);
		break;

	case EDlgEventType::Event:
	default:
		Dialogue->GetEvents(AttendeeName, Suggestions);
		break;
	}

	FDlgHelper::SortDefault(Suggestions);
	return Suggestions.Array();
}

#undef LOCTEXT_NAMESPACE
