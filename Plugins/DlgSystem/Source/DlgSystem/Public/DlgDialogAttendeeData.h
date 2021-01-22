
#pragma once

#include "DlgDialogAttendeeData.generated.h"

struct FDlgCondition;
struct FDlgEvent;
struct FDlgTextArgument;

// Structure useful to cache all the names used by a attendee
USTRUCT(BlueprintType)
struct DLGSYSTEM_API FDlgAttendeeData
{
	GENERATED_USTRUCT_BODY()

public:
	// Helper functions to fill the data
	void AddConditionPrimaryData(const FDlgCondition& Condition);
	void AddConditionSecondaryData(const FDlgCondition& Condition);
	void AddEventData(const FDlgEvent& Event);
	void AddTextArgumentData(const FDlgTextArgument& TextArgument);

public:
	// FName based conditions (aka conditions of type EventCall).
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> Conditions;

	// Custom Conditions UClasses of type UDlgConditionCustom
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<UClass*> CustomConditions;

	// FName based events (aka events of type EDlgEventType)
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> Events;

	// Custom Events UClasses of type UDlgEventCustom
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<UClass*> CustomEvents;

	// Custom Events UClasses of type UDlgTextArgumentCustom
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<UClass*> CustomTextArguments;

	// Integers used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> IntVariableNames;

	// Floats used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> FloatVariableNames;

	// Booleans used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> BoolVariableNames;

	// Names used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> NameVariableNames;

	// Class Integers used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> ClassIntVariableNames;

	// Class Floats used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> ClassFloatVariableNames;

	// Class Booleans used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> ClassBoolVariableNames;

	// Class Names used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> ClassNameVariableNames;

	// Class Texts used in a Dialogue
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Dialogue|Attendee")
	TSet<FName> ClassTextVariableNames;
};
