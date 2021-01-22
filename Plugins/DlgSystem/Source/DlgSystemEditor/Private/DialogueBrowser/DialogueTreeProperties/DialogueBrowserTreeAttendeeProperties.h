
#pragma once
#include "CoreMinimal.h"

#include "DialogueBrowserTreeVariableProperties.h"
#include "TreeViewHelpers/DlgTreeViewAttendeeProperties.h"

/** Used as a key in the fast lookup table. */
class FDialogueBrowserTreeAttendeeProperties : public FDlgTreeViewAttendeeProperties<FDialogueBrowserTreeVariableProperties>
{
	typedef FDialogueBrowserTreeAttendeeProperties Self;
	typedef FDlgTreeViewAttendeeProperties Super;

public:
	FDialogueBrowserTreeAttendeeProperties(const TSet<TWeakObjectPtr<const UDlgDialogue>>& InDialogues) : Super(InDialogues) {}
	FDialogueBrowserTreeAttendeeProperties(const TSet<TWeakObjectPtr<const UDlgDialogue>>&& InDialogues) : Super(InDialogues) {}
};
