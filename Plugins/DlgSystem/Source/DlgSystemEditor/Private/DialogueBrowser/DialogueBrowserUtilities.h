
#pragma once

#include "CoreMinimal.h"

#include "DialogueBrowserTreeNode.h"
#include "DialogueTreeProperties/DialogueBrowserTreeAttendeeProperties.h"

enum class EDialogueBrowserSortOption : uint8
{
	Name = 0,
	DialogueReferences
};

struct FDialogueBrowserSortOption
{

public:
	FDialogueBrowserSortOption(EDialogueBrowserSortOption InOption, FName InName)
		: Option(InOption), Name(InName) {}

	FName GetFName() const { return Name; }
	FText GetFText() const { return FText::FromName(Name); }
	FString GetFString() const { return Name.ToString(); }

	bool IsByName() const { return Option == EDialogueBrowserSortOption::Name; }

public:
	EDialogueBrowserSortOption Option;

	// The name of the option.
	FName Name;

	// TODO add ascending descending
};

class FDialogueBrowserUtilities
{
public:
	// Compare two FDialogueBrowserTreeNode
	static bool PredicateCompareDialogueTreeNode(
		const TSharedPtr<FDialogueBrowserTreeNode>& FirstNode,
		const TSharedPtr<FDialogueBrowserTreeNode> SecondNode
	)
	{
		check(FirstNode.IsValid());
		check(SecondNode.IsValid());
		return *FirstNode == *SecondNode;
	}

	// Predicate that sorts attendees by dialogue number references, in descending order.
	static bool PredicateSortByDialoguesNumDescending(
		FName FirstAttendee,
		FName SecondAttendee,
		const TMap<FName, TSharedPtr<FDialogueBrowserTreeAttendeeProperties>>& AttendeesProperties
	)
	{
		int32 FirstNum = 0;
		int32 SecondNum = 0;

		const TSharedPtr<FDialogueBrowserTreeAttendeeProperties>* FirstPtr =
			AttendeesProperties.Find(FirstAttendee);
		if (FirstPtr)
		{
			FirstNum = (*FirstPtr)->GetDialogues().Num();
		}
		const TSharedPtr<FDialogueBrowserTreeAttendeeProperties>* SecondPtr =
			AttendeesProperties.Find(SecondAttendee);
		if (SecondPtr)
		{
			SecondNum = (*SecondPtr)->GetDialogues().Num();
		}

		return FirstNum > SecondNum;
	}
};
