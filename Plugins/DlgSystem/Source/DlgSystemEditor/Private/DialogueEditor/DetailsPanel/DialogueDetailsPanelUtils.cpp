
#include "DialogueDetailsPanelUtils.h"
#include "DlgHelper.h"


UDialogueGraphNode_Base* FDialogueDetailsPanelUtils::GetGraphNodeBaseFromPropertyHandle(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	for (UObject* Object : OuterObjects)
	{
		if (UDlgNode* Node = Cast<UDlgNode>(Object))
		{
			return CastChecked<UDialogueGraphNode_Base>(Node->GetGraphNode());
		}

		if (UDialogueGraphNode_Base* Node = Cast<UDialogueGraphNode_Base>(Object))
		{
			return Node;
		}
	}

	return nullptr;
}

UDialogueGraphNode* FDialogueDetailsPanelUtils::GetClosestGraphNodeFromPropertyHandle(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	if (UDialogueGraphNode_Base* BaseGraphNode = GetGraphNodeBaseFromPropertyHandle(PropertyHandle))
	{
		if (UDialogueGraphNode* Node = Cast<UDialogueGraphNode>(BaseGraphNode))
		{
			return Node;
		}
		if (UDialogueGraphNode_Edge* GraphEdge = Cast<UDialogueGraphNode_Edge>(BaseGraphNode))
		{
			if (GraphEdge->HasParentNode())
			{
				return GraphEdge->GetParentNode();
			}
		}
	}

	return nullptr;
}

UDlgDialogue* FDialogueDetailsPanelUtils::GetDialogueFromPropertyHandle(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	UDlgDialogue* Dialogue = nullptr;

	// Check first children objects of property handle, should be a dialogue node or a graph node
	if (UDialogueGraphNode_Base* GraphNode = GetGraphNodeBaseFromPropertyHandle(PropertyHandle))
	{
		Dialogue = GraphNode->GetDialogue();
	}

	// One last try, get to the root of the problem ;)
	if (!IsValid(Dialogue))
	{
		TSharedPtr<IPropertyHandle> ParentHandle = PropertyHandle->GetParentHandle();
		// Find the root property handle
		while (ParentHandle.IsValid() && ParentHandle->GetParentHandle().IsValid())
		{
			ParentHandle = ParentHandle->GetParentHandle();
		}

		// The outer should be a dialogue
		if (ParentHandle.IsValid())
		{
			TArray<UObject*> OuterObjects;
			ParentHandle->GetOuterObjects(OuterObjects);
			for (UObject* Object : OuterObjects)
			{
				if (UDlgDialogue* FoundDialogue = Cast<UDlgDialogue>(Object))
				{
					Dialogue = FoundDialogue;
					break;
				}
			}
		}
	}

	return Dialogue;
}

FName FDialogueDetailsPanelUtils::GetAttendeeNameFromPropertyHandle(const TSharedRef<IPropertyHandle>& AttendeeNamePropertyHandle)
{
	FName AttendeeName = NAME_None;
	if (AttendeeNamePropertyHandle->GetValue(AttendeeName) != FPropertyAccess::Success)
	{
		return AttendeeName;
	}

	// Try the node that owns this
	if (AttendeeName.IsNone())
	{
		// Possible edge?
		if (UDialogueGraphNode* GraphNode = GetClosestGraphNodeFromPropertyHandle(AttendeeNamePropertyHandle))
		{
			return GraphNode->GetDialogueNode().GetNodeAttendeeName();
		}
	}

	return AttendeeName;
}

TArray<FName> FDialogueDetailsPanelUtils::GetDialogueSortedAttendeeNames(UDlgDialogue* Dialogue)
{
	if (Dialogue == nullptr)
	{
		return {};
	}

	TSet<FName> AttendeeNames;
	Dialogue->GetAllAttendeeNames(AttendeeNames);
	FDlgHelper::SortDefault(AttendeeNames);
	return AttendeeNames.Array();
}
