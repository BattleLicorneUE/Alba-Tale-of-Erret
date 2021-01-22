
#pragma once

#include "CoreMinimal.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "DlgDialogAttendee.h"
#include "DlgManager.h"

class FDialogueBlueprintUtilities
{
public:
	// Gets the blueprint for the provided Node
	static UBlueprint* GetBlueprintForGraphNode(const UK2Node* Node)
	{
		if (!IsValid(Node))
		{
			return nullptr;
		}

		// NOTE we can't call Node->GetBlueprint() because this is called in strange places ;)
		if (const UEdGraph* Graph = Cast<UEdGraph>(Node->GetOuter()))
		{
			return FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
		}

		return nullptr;
	}

	// Checks if the Blueprint for the Node is loaded or not.
	static bool IsBlueprintLoadedForGraphNode(const UK2Node* Node)
	{
		if (UBlueprint* Blueprint = GetBlueprintForGraphNode(Node))
		{
			return !Blueprint->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad);
		}
		return false;
	}

	/**
	 * Tries to get the dialogue name... it expects the owner of the node to implement IDlgDialogAttendee interface
	 * @return		the attendee name on success or NAME_None on failure.
	 */
	static FName GetAttendeeNameFromNode(const UK2Node* Node)
	{
		if (const UBlueprint* Blueprint = GetBlueprintForGraphNode(Node))
		{
			if (UDlgManager::DoesObjectImplementDialogueAttendeeInterface(Blueprint))
			{
				return IDlgDialogAttendee::Execute_GetAttendeeName(Blueprint->GeneratedClass->GetDefaultObject());
			}
		}

		return NAME_None;
	}
};
