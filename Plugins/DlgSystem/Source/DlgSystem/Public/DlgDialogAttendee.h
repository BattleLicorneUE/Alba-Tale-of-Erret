
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"

#include "DlgDialogAttendee.generated.h"

class UTexture2D;
class UDlgContext;

UINTERFACE(BlueprintType, Blueprintable)
class DLGSYSTEM_API UDlgDialogAttendee : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};
inline UDlgDialogAttendee::UDlgDialogAttendee(const FObjectInitializer& ObjectInitializer) {}

/**
 * Interface that each Dialogue attendee must implement
 */
class DLGSYSTEM_API IDlgDialogAttendee
{
	GENERATED_IINTERFACE_BODY()

	//
	// Attendee information
	//

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee")
	FName GetAttendeeName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee")
	FText GetAttendeeDisplayName(FName ActiveSpeaker) const;

	/** May be used for formatted node texts, check https://docs.unrealengine.com/en-us/Gameplay/Localization/Formatting for more information */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee")
	ETextGender GetAttendeeGender() const;

	/**
	* @param	ActiveSpeaker: name of the active speaker at the time of the call (might or might not this attendee)
	* @param	ActiveSpeakerState: state of the active attendee (might or might not belong to this attendee)
	*			If it is not displayed in editor it has to be turned on in the dialogue settings
	* @return	Attendee icon to display
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee")
	UTexture2D* GetAttendeeIcon(FName ActiveSpeaker, FName ActiveSpeakerState) const;

	//
	// Conditions
	//

	// Used for the condition type EDlgConditionType::EventCall (Check Dialogue Named Condition)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Condition")
	bool CheckCondition(const UDlgContext* Context, FName ConditionName) const;

	// Used for the condition type EDlgConditionType::FloatCall (Check Dialogue Float Value)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Condition")
	float GetFloatValue(FName ValueName) const;

	// Used for the condition type EDlgConditionType::IntCall (Check Dialogue Int Value)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Condition")
	int32 GetIntValue(FName ValueName) const;

	// Used for the condition type EDlgConditionType::BoolCall (Check Dialogue Bool Value)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Condition")
	bool GetBoolValue(FName ValueName) const;

	// Used for the condition type EDlgConditionType::NameCall (Check Dialogue Name Value)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Condition")
	FName GetNameValue(FName ValueName) const;

	//
	// Events
	//

	// Used for the event type EDlgEventType::Event (Event)
	// @return value: irrelevant
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Event")
	bool OnDialogueEvent(UDlgContext* Context, FName EventName);

	// Used for the event type EDlgEventType::ModifyFloat (Modify Dialogue Float Value)
	// @param	bDelta Whether we expect the value to be set or modified
	// @return	Irrelevant, ignored
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Event")
	bool ModifyFloatValue(FName ValueName, bool bDelta, float Value);

	// Used for the event type EDlgEventType::ModifyInt (Modify Dialogue Int Value)
	// @param	bDelta Whether we expect the value to be set or modified
	// @return	Irrelevant, ignored
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Event")
	bool ModifyIntValue(FName ValueName, bool bDelta, int32 Value);

	// Used for the event type EDlgEventType::ModifyBool (Modify Dialogue Bool Value)
	// @return value: irrelevant
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Event")
	bool ModifyBoolValue(FName ValueName, bool bNewValue);

	// Used for the event type EDlgEventType::ModifyName (Modify Dialogue Name Value)
	// @return value: irrelevant
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Attendee|Event")
	bool ModifyNameValue(FName ValueName, FName NameValue);
};
