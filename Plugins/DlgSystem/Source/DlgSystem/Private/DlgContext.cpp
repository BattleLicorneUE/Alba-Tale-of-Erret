
#include "DlgContext.h"

#include "DlgConstants.h"
#include "Nodes/DlgNode.h"
#include "Nodes/DlgNode_End.h"
#include "Nodes/DlgNode_SpeechSequence.h"
#include "DlgDialogAttendee.h"
#include "DlgMemory.h"
#include "Engine/Texture2D.h"
#include "Logging/DlgLogger.h"
#include "Net/UnrealNetwork.h"


UDlgContext::UDlgContext(const FObjectInitializer& ObjectInitializer)
	: UDlgObject(ObjectInitializer)
{
	//UObject.bReplicates = true;
}

void UDlgContext::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Dialogue);
	DOREPLIFETIME(ThisClass, SerializedAttendees);
}

void UDlgContext::SerializeAttendees()
{
	SerializedAttendees.Empty(Attendees.Num());
	for (const auto& KeyValue : Attendees)
	{
		SerializedAttendees.Add(KeyValue.Value);
	}
}

void UDlgContext::OnRep_SerializedAttendees()
{
	Attendees.Empty(SerializedAttendees.Num());
	for (UObject* Attendee : SerializedAttendees)
	{
		if (IsValid(Attendee))
		{
			Attendees.Add(IDlgDialogAttendee::Execute_GetAttendeeName(Attendee), Attendee);
		}
	}
}

bool UDlgContext::ChooseOption(int32 OptionIndex)
{
	check(Dialogue);
	if (UDlgNode* Node = GetMutableActiveNode())
	{
		if (Node->OptionSelected(OptionIndex, *this))
		{
			return true;
		}
	}

	bDialogueEnded = true;
	return false;
}

bool UDlgContext::ChooseSpeechSequenceOptionFromReplicated(int32 OptionIndex)
{
	check(Dialogue);
	if (UDlgNode_SpeechSequence* Node = GetMutableActiveNodeAsSpeechSequence())
	{
		if (Node->OptionSelectedFromReplicated(OptionIndex, *this))
		{
			return true;
		}
	}

	bDialogueEnded = true;
	return false;
}

bool UDlgContext::ChooseOptionBasedOnAllOptionIndex(int32 Index)
{
	if (!AllChildren.IsValidIndex(Index))
	{
		LogErrorWithContext(FString::Printf(TEXT("ChooseOptionBasedOnAllOptionIndex - INVALID given Index = %d"), Index));
		bDialogueEnded = true;
		return false;
	}

	if (!AllChildren[Index].IsSatisfied())
	{
		LogErrorWithContext(FString::Printf(TEXT("ChooseOptionBasedOnAllOptionIndex - given Index = %d is an unsatisfied edge"), Index));
		bDialogueEnded = true;
		return false;
	}

	for (int32 i = 0; i < AvailableChildren.Num(); ++i)
	{
		if (AvailableChildren[i] == AllChildren[Index].GetEdge())
		{
			return ChooseOption(i);
		}
	}

	ensure(false);
	bDialogueEnded = true;
	return false;
}

bool UDlgContext::ReevaluateOptions()
{
	check(Dialogue);
	UDlgNode* Node = GetMutableActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("ReevaluateOptions - Failed to update dialogue options"));
		return false;
	}

	return Node->ReevaluateChildren(*this, {});
}

const FText& UDlgContext::GetOptionText(int32 OptionIndex) const
{
	check(Dialogue);
	if (!AvailableChildren.IsValidIndex(OptionIndex))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionText - INVALID given OptionIndex = %d"), OptionIndex));
		return FText::GetEmpty();
	}

	return AvailableChildren[OptionIndex].GetText();
}

FName UDlgContext::GetOptionSpeakerState(int32 OptionIndex) const
{
	check(Dialogue);
	if (!AvailableChildren.IsValidIndex(OptionIndex))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionSpeakerState - INVALID given OptionIndex = %d"), OptionIndex));
		return NAME_None;
	}

	return AvailableChildren[OptionIndex].SpeakerState;
}

const TArray<FDlgCondition>& UDlgContext::GetOptionEnterConditions(int32 OptionIndex) const
{
	check(Dialogue);
	if (!AvailableChildren.IsValidIndex(OptionIndex))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionEnterConditions - INVALID given OptionIndex = %d"), OptionIndex));
		static TArray<FDlgCondition> EmptyArray;
		return EmptyArray;
	}

	return AvailableChildren[OptionIndex].Conditions;
}

const FDlgEdge& UDlgContext::GetOption(int32 OptionIndex) const
{
	check(Dialogue);
	if (!AvailableChildren.IsValidIndex(OptionIndex))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOption - INVALID given OptionIndex = %d"), OptionIndex));
		return FDlgEdge::GetInvalidEdge();
	}

	return AvailableChildren[OptionIndex];
}

const FText& UDlgContext::GetOptionTextFromAll(int32 Index) const
{
	check(Dialogue);
	if (!AllChildren.IsValidIndex(Index))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionTextFromAll - INVALID given Index = %d"), Index));
		return FText::GetEmpty();
	}

	return AllChildren[Index].GetEdge().GetText();
}

bool UDlgContext::IsOptionSatisfied(int32 Index) const
{
	check(Dialogue);
	if (!AllChildren.IsValidIndex(Index))
	{
		LogErrorWithContext(FString::Printf(TEXT("IsOptionSatisfied - INVALID given Index = %d"), Index));
		return false;
	}

	return AllChildren[Index].IsSatisfied();
}

FName UDlgContext::GetOptionSpeakerStateFromAll(int32 Index) const
{
	check(Dialogue);
	if (!AllChildren.IsValidIndex(Index))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionSpeakerStateFromAll - INVALID given Index = %d"), Index));
		return NAME_None;
	}

	return AllChildren[Index].GetEdge().SpeakerState;
}

const FDlgEdgeData& UDlgContext::GetOptionFromAll(int32 Index) const
{
	check(Dialogue);
	if (!AvailableChildren.IsValidIndex(Index))
	{
		LogErrorWithContext(FString::Printf(TEXT("GetOptionFromAll - INVALID given Index = %d"), Index));
		return FDlgEdgeData::GetInvalidEdge();
	}

	return AllChildren[Index];
}

const FText& UDlgContext::GetActiveNodeText() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeText - INVALID Active Node"));
		return FText::GetEmpty();
	}

	return Node->GetNodeText();
}

FName UDlgContext::GetActiveNodeSpeakerState() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeSpeakerState - INVALID Active Node"));
		return NAME_None;
	}

	return Node->GetSpeakerState();
}

USoundWave* UDlgContext::GetActiveNodeVoiceSoundWave() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeVoiceSoundWave - INVALID Active Node"));
		return nullptr;
	}

	return Node->GetNodeVoiceSoundWave();
}

USoundBase* UDlgContext::GetActiveNodeVoiceSoundBase() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeVoiceSoundBase - INVALID Active Node"));
		return nullptr;
	}

	return Node->GetNodeVoiceSoundBase();
}

UDialogueWave* UDlgContext::GetActiveNodeVoiceDialogueWave() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeVoiceDialogueWave - INVALID Active Node"));
		return nullptr;
	}

	return Node->GetNodeVoiceDialogueWave();
}

UObject* UDlgContext::GetActiveNodeGenericData() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeGenericData - INVALID Active Node"));
		return nullptr;
	}

	return Node->GetNodeGenericData();
}

UDlgNodeData* UDlgContext::GetActiveNodeData() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeData - INVALID Active Node"));
		return nullptr;
	}

	return Node->GetNodeData();
}

UTexture2D* UDlgContext::GetActiveNodeAttendeeIcon() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeAttendeeIcon - INVALID Active Node"));
		return nullptr;
	}

	const FName SpeakerName = Node->GetNodeAttendeeName();
	auto* ObjectPtr = Attendees.Find(SpeakerName);
	if (ObjectPtr == nullptr || !IsValid(*ObjectPtr))
	{
		LogErrorWithContext(FString::Printf(
			TEXT("GetActiveNodeAttendeeIcon - The AttendeeName = `%s` from the Active Node does NOT exist in the current Attendees"),
			*SpeakerName.ToString()
		));
		return nullptr;
	}

	return IDlgDialogAttendee::Execute_GetAttendeeIcon(*ObjectPtr, SpeakerName, Node->GetSpeakerState());
}

UObject* UDlgContext::GetActiveNodeAttendee() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeAttendee - INVALID Active Node"));
		return nullptr;
	}

	const FName SpeakerName = Node->GetNodeAttendeeName();
	auto* ObjectPtr = Attendees.Find(Node->GetNodeAttendeeName());
	if (ObjectPtr == nullptr || !IsValid(*ObjectPtr))
	{
		LogErrorWithContext(FString::Printf(
			TEXT("GetActiveNodeAttendee - The AttendeeName = `%s` from the Active Node does NOT exist in the current Attendees"),
			*SpeakerName.ToString()
		));
		return nullptr;
	}

	return *ObjectPtr;
}

FName UDlgContext::GetActiveNodeAttendeeName() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeAttendeeName - INVALID Active Node"));
		return NAME_None;
	}

	return Node->GetNodeAttendeeName();
}

FText UDlgContext::GetActiveNodeAttendeeDisplayName() const
{
	const UDlgNode* Node = GetActiveNode();
	if (!IsValid(Node))
	{
		LogErrorWithContext(TEXT("GetActiveNodeAttendeeDisplayName - INVALID Active Node"));
		return FText::GetEmpty();
	}

	const FName SpeakerName = Node->GetNodeAttendeeName();
	auto* ObjectPtr = Attendees.Find(SpeakerName);
	if (ObjectPtr == nullptr || !IsValid(*ObjectPtr))
	{
		LogErrorWithContext(FString::Printf(
			TEXT("GetActiveNodeAttendeeDisplayName - The AttendeeName = `%s` from the Active Node does NOT exist in the current Attendees"),
			*SpeakerName.ToString()
		));
		return FText::GetEmpty();
	}

	return IDlgDialogAttendee::Execute_GetAttendeeDisplayName(*ObjectPtr, SpeakerName);
}

UObject* UDlgContext::GetMutableAttendee(FName AttendeeName) const
{
	auto* AttendeePtr = Attendees.Find(AttendeeName);
	if (AttendeePtr != nullptr && IsValid(*AttendeePtr))
	{
		return *AttendeePtr;
	}

	return nullptr;
}

const UObject* UDlgContext::GetAttendee(FName AttendeeName) const
{
	auto* AttendeePtr = Attendees.Find(AttendeeName);
	if (AttendeePtr != nullptr && IsValid(*AttendeePtr))
	{
		return *AttendeePtr;
	}

	return nullptr;
}

bool UDlgContext::IsValidNodeIndex(int32 NodeIndex) const
{
	return Dialogue ? Dialogue->IsValidNodeIndex(NodeIndex) : false;
}

bool UDlgContext::IsValidNodeGUID(const FGuid& NodeGUID) const
{
	return Dialogue ? Dialogue->IsValidNodeGUID(NodeGUID) : false;
}

FGuid UDlgContext::GetNodeGUIDForIndex(int32 NodeIndex) const
{
	return Dialogue ? Dialogue->GetNodeGUIDForIndex(NodeIndex) : FGuid{};
}

int32 UDlgContext::GetNodeIndexForGUID(const FGuid& NodeGUID) const
{
	return Dialogue ? Dialogue->GetNodeIndexForGUID(NodeGUID) : INDEX_NONE;
}

bool UDlgContext::IsOptionConnectedToVisitedNode(int32 Index, bool bLocalHistory, bool bIndexSkipsUnsatisfiedEdges) const
{
	int32 TargetIndex = INDEX_NONE;

	if (bIndexSkipsUnsatisfiedEdges)
	{
		if (!AvailableChildren.IsValidIndex(Index))
		{
			LogErrorWithContext(FString::Printf(TEXT("IsOptionConnectedToVisitedNode - INVALID Index = %d for AvailableChildren"), Index));
			return false;
		}
		TargetIndex = AvailableChildren[Index].TargetIndex;
	}
	else
	{
		if (!AllChildren.IsValidIndex(Index))
		{
			LogErrorWithContext(FString::Printf(TEXT("IsOptionConnectedToVisitedNode - INVALID Index = %d for AllChildren"), Index));
			return false;
		}
		TargetIndex = AllChildren[Index].GetEdge().TargetIndex;
	}

	const FGuid TargetGUID = GetNodeGUIDForIndex(TargetIndex);
	if (bLocalHistory)
	{
		return History.Contains(TargetIndex, TargetGUID);
	}

	if (Dialogue == nullptr)
	{
		LogErrorWithContext(TEXT("IsOptionConnectedToVisitedNode - This Context does not have a valid Dialogue"));
		return false;
	}

	return FDlgMemory::Get().IsNodeVisited(Dialogue->GetGUID(), TargetIndex, TargetGUID);
}

bool UDlgContext::IsOptionConnectedToEndNode(int32 Index, bool bIndexSkipsUnsatisfiedEdges) const
{
	int32 TargetIndex = INDEX_NONE;

	if (bIndexSkipsUnsatisfiedEdges)
	{
		if (!AvailableChildren.IsValidIndex(Index))
		{
			LogErrorWithContext(FString::Printf(TEXT("IsOptionConnectedToEndNode - INVALID Index = %d for AvailableChildren"), Index));
			return false;
		}
		TargetIndex = AvailableChildren[Index].TargetIndex;
	}
	else
	{
		if (!AllChildren.IsValidIndex(Index))
		{
			LogErrorWithContext(FString::Printf(TEXT("IsOptionConnectedToEndNode - INVALID Index = %d for AllChildren"), Index));
			return false;
		}
		TargetIndex = AllChildren[Index].GetEdge().TargetIndex;
	}

	if (Dialogue == nullptr)
	{
		LogErrorWithContext(TEXT("IsOptionConnectedToEndNode - This Context does not have a valid Dialogue"));
		return false;
	}

	const TArray<UDlgNode*>& Nodes = Dialogue->GetNodes();
	if (Nodes.IsValidIndex(TargetIndex))
	{
		return Nodes[TargetIndex]->IsA<UDlgNode_End>();
	}

	LogErrorWithContext(FString::Printf(TEXT("IsOptionConnectedToEndNode - The examined Edge/Option at Index = %d does not point to a valid node"), Index));
	return false;
}

bool UDlgContext::EnterNode(int32 NodeIndex, TSet<const UDlgNode*> NodesEnteredWithThisStep)
{
	check(Dialogue);
	UDlgNode* Node = GetMutableNodeFromIndex(NodeIndex);
	if (!IsValid(Node))
	{
		LogErrorWithContext(FString::Printf(TEXT("EnterNode - FAILED because of INVALID NodeIndex = %d"), NodeIndex));
		return false;
	}

	ActiveNodeIndex = NodeIndex;
	SetNodeVisited(NodeIndex, Node->GetGUID());

	return Node->HandleNodeEnter(*this, NodesEnteredWithThisStep);
}

UDlgContext* UDlgContext::CreateCopy() const
{
	UObject* FirstAttendee = nullptr;
	for (const auto& KeyValue : Attendees)
	{
		if (KeyValue.Value)
		{
			FirstAttendee = KeyValue.Value;
			break;
		}
	}
	if (!FirstAttendee)
	{
		return nullptr;
	}

	auto* Context = NewObject<UDlgContext>(FirstAttendee, StaticClass());
	Context->Dialogue = Dialogue;
	Context->SetAttendees(Attendees);
	Context->ActiveNodeIndex = ActiveNodeIndex;
	Context->AvailableChildren = AvailableChildren;
	Context->AllChildren = AllChildren;
	Context->History = History;
	Context->bDialogueEnded = bDialogueEnded;

	return Context;
}

void UDlgContext::SetNodeVisited(int32 NodeIndex, const FGuid& NodeGUID)
{
	FDlgMemory::Get().SetNodeVisited(Dialogue->GetGUID(), NodeIndex, NodeGUID);
	History.Add(NodeIndex, NodeGUID);
}

UDlgNode_SpeechSequence* UDlgContext::GetMutableActiveNodeAsSpeechSequence() const
{
	return Cast<UDlgNode_SpeechSequence>(GetMutableNodeFromIndex(ActiveNodeIndex));
}

const UDlgNode_SpeechSequence* UDlgContext::GetActiveNodeAsSpeechSequence() const
{
	return Cast<UDlgNode_SpeechSequence>(GetNodeFromIndex(ActiveNodeIndex));
}

UDlgNode* UDlgContext::GetMutableNodeFromIndex(int32 NodeIndex) const
{
	check(Dialogue);
	if (!Dialogue->IsValidNodeIndex(NodeIndex))
	{
		return nullptr;
	}

	return Dialogue->GetMutableNodeFromIndex(NodeIndex);
}

const UDlgNode* UDlgContext::GetNodeFromIndex(int32 NodeIndex) const
{
	check(Dialogue);
	if (!Dialogue->IsValidNodeIndex(NodeIndex))
	{
		return nullptr;
	}

	return Dialogue->GetMutableNodeFromIndex(NodeIndex);
}

UDlgNode* UDlgContext::GetMutableNodeFromGUID(const FGuid& NodeGUID) const
{
	check(Dialogue);
	if (!Dialogue->IsValidNodeGUID(NodeGUID))
	{
		return nullptr;
	}

	return Dialogue->GetMutableNodeFromGUID(NodeGUID);
}

const UDlgNode* UDlgContext::GetNodeFromGUID(const FGuid& NodeGUID) const
{
	check(Dialogue);
	if (!Dialogue->IsValidNodeGUID(NodeGUID))
	{
		return nullptr;
	}

	return Dialogue->GetMutableNodeFromGUID(NodeGUID);
}

bool UDlgContext::IsNodeEnterable(int32 NodeIndex, TSet<const UDlgNode*> AlreadyVisitedNodes) const
{
	check(Dialogue);
	if (const UDlgNode* Node = GetNodeFromIndex(NodeIndex))
	{
		return Node->CheckNodeEnterConditions(*this, AlreadyVisitedNodes);
	}

	return false;
}

bool UDlgContext::CanBeStarted(UDlgDialogue* InDialogue, const TMap<FName, UObject*>& InAttendees)
{
	if (!ValidateAttendeesMapForDialogue(TEXT("CanBeStarted"), InDialogue, InAttendees, false))
	{
		return false;
	}

	// Get first attendee
	UObject* FirstAttendee = nullptr;
	for (const auto& KeyValue : InAttendees)
	{
		if (KeyValue.Value)
		{
			FirstAttendee = KeyValue.Value;
			break;
		}
	}
	check(FirstAttendee != nullptr);

	// Create temporary context that is Garbage Collected after this function returns (hopefully)
	auto* Context = NewObject<UDlgContext>(FirstAttendee, StaticClass());
	Context->Dialogue = InDialogue;
	Context->SetAttendees(InAttendees);

	// Evaluate edges/children of the start node
	const UDlgNode& StartNode = InDialogue->GetStartNode();
	for (const FDlgEdge& ChildLink : StartNode.GetNodeChildren())
	{
		if (ChildLink.Evaluate(*Context, {}))
		{
			// Simulate EnterNode
			UDlgNode* Node = Context->GetMutableNodeFromIndex(ChildLink.TargetIndex);
			if (Node && Node->HasAnySatisfiedChild(*Context, {}))
			{
				return true;
			}
		}
	}

	return false;
}

bool UDlgContext::StartWithContext(const FString& ContextString, UDlgDialogue* InDialogue, const TMap<FName, UObject*>& InAttendees)
{
	const FString ContextMessage = ContextString.IsEmpty()
		? TEXT("Start")
		: FString::Printf(TEXT("%s - Start"), *ContextString);

	Dialogue = InDialogue;
	SetAttendees(InAttendees);
	if (!ValidateAttendeesMapForDialogue(ContextMessage, Dialogue, Attendees))
	{
		return false;
	}

	// Evaluate edges/children of the start node
	const UDlgNode& StartNode = Dialogue->GetStartNode();
	for (const FDlgEdge& ChildLink : StartNode.GetNodeChildren())
	{
		if (ChildLink.Evaluate(*this, {}))
		{
			if (EnterNode(ChildLink.TargetIndex, {}))
			{
				return true;
			}
		}
	}

	LogErrorWithContext(FString::Printf(
		TEXT("%s - FAILED because all possible start node condition failed. Edge conditions and children enter conditions from the start node are not satisfied"),
		*ContextMessage
	));
	return false;
}

bool UDlgContext::StartWithContextFromNode(
	const FString& ContextString,
	UDlgDialogue* InDialogue,
	const TMap<FName, UObject*>& InAttendees,
	int32 StartNodeIndex,
	const FGuid& StartNodeGUID,
	const FDlgHistory& StartHistory,
	bool bFireEnterEvents
)
{
	const FString ContextMessage = ContextString.IsEmpty()
		? TEXT("StartFromNode")
		: FString::Printf(TEXT("%s - StartFromNode"), *ContextString);

	Dialogue = InDialogue;
	SetAttendees(InAttendees);
	History = StartHistory;
	if (!ValidateAttendeesMapForDialogue(ContextMessage, Dialogue, Attendees))
	{
		return false;
	}

	// Get the StartNodeIndex from the GUID
	if (StartNodeGUID.IsValid())
	{
		StartNodeIndex = GetNodeIndexForGUID(StartNodeGUID);
	}

	UDlgNode* Node = GetMutableNodeFromIndex(StartNodeIndex);
	if (!IsValid(Node))
	{
		LogErrorWithContext(FString::Printf(
			TEXT("%s - FAILED because StartNodeIndex = %d  is INVALID. For StartNodeGUID = %s"),
			*ContextMessage, StartNodeIndex, *StartNodeGUID.ToString()
		));
		return false;
	}

	if (bFireEnterEvents)
	{
		return EnterNode(StartNodeIndex, {});
	}

	ActiveNodeIndex = StartNodeIndex;
	SetNodeVisited(StartNodeIndex, Node->GetGUID());

	return Node->ReevaluateChildren(*this, {});
}

FString UDlgContext::GetContextString() const
{
	FString ContextAttendees;
	TSet<FString> AttendeesNames;
	for (const auto& KeyValue : Attendees)
	{
		AttendeesNames.Add(KeyValue.Key.ToString());
	}

	return FString::Printf(
		TEXT("Dialogue = `%s`, ActiveNodeIndex = %d, Attendees Names = `%s`"),
		Dialogue ? *Dialogue->GetPathName() : TEXT("INVALID"),
		ActiveNodeIndex,
		*FString::Join(AttendeesNames, TEXT(", "))
	);
}

void UDlgContext::LogErrorWithContext(const FString& ErrorMessage) const
{
	FDlgLogger::Get().Error(GetErrorMessageWithContext(ErrorMessage));
}

FString UDlgContext::GetErrorMessageWithContext(const FString& ErrorMessage) const
{
	return FString::Printf(TEXT("%s.\nContext:\n\t%s"), *ErrorMessage, *GetContextString());
}

EDlgValidateStatus UDlgContext::IsValidAttendeeForDialogue(const UDlgDialogue* Dialogue, const UObject* Attendee)
{
	if (!IsValid(Attendee))
	{
		return EDlgValidateStatus::AttendeeIsNull;
	}
	if (!IsValid(Dialogue))
	{
		return EDlgValidateStatus::DialogueIsNull;
	}

	// Does not implement interface
	if (!Attendee->GetClass()->ImplementsInterface(UDlgDialogAttendee::StaticClass()))
	{
		if (Attendee->IsA<UBlueprint>())
		{
			return EDlgValidateStatus::AttendeeIsABlueprintClassAndDoesNotImplementInterface;
		}

		return EDlgValidateStatus::AttendeeDoesNotImplementInterface;
	}

	// We are more relaxed about this
	// Even if the user supplies more attendees that required we still allow them to start the dialogue if the number of attendees is bigger

	// Does the attendee name exist in the Dialogue?
	// const FName AttendeeName = IDlgDialogAttendee::Execute_GetAttendeeName(Attendee);
	// if (!Dialogue->HasAttendee(AttendeeName))
	// {
	// 	return EDlgValidateStatus::DialogueDoesNotContainAttendee;
	// }

	return EDlgValidateStatus::Valid;
}

bool UDlgContext::ValidateAttendeeForDialogue(
	const FString& ContextString,
	const UDlgDialogue* Dialogue,
	const UObject* Attendee,
	bool bLog
)
{
	const EDlgValidateStatus Status = IsValidAttendeeForDialogue(Dialogue, Attendee);

	// Act as IsValidAttendeeForDialogue
	if (!bLog)
	{
		return Status == EDlgValidateStatus::Valid;
	}

	switch (Status)
	{
		case EDlgValidateStatus::Valid:
			return true;

		case EDlgValidateStatus::DialogueIsNull:
			FDlgLogger::Get().Errorf(
				TEXT("%s - Dialogue is INVALID (not set or null).\nContext:\n\tAttendee = `%s`"),
				*ContextString, Attendee ? *Attendee->GetPathName() : TEXT("INVALID")
			);
			return false;

		case EDlgValidateStatus::AttendeeIsNull:
			FDlgLogger::Get().Errorf(
				TEXT("%s - Attendee is INVALID (not set or null).\nContext:\n\tDialogue = `%s`"),
				*ContextString, Dialogue ? *Dialogue->GetPathName() : TEXT("INVALID")
			);
			return false;

		case EDlgValidateStatus::AttendeeDoesNotImplementInterface:
			FDlgLogger::Get().Errorf(
				TEXT("%s - Attendee Path = `%s` does not implement the IDlgDialogAttendee/UDlgDialogAttendee interface.\nContext:\n\tDialogue = `%s`"),
				*ContextString, *Attendee->GetPathName(), *Dialogue->GetPathName()
			);
			return false;

		case EDlgValidateStatus::AttendeeIsABlueprintClassAndDoesNotImplementInterface:
			FDlgLogger::Get().Errorf(
				TEXT("%s - Attendee Path = `%s` is a Blueprint Class (from the content browser) and NOT a Blueprint Instance (from the level world).\nContext:\n\tDialogue = `%s`"),
				*ContextString, *Attendee->GetPathName(), *Dialogue->GetPathName()
			);
			return false;

		// case EDlgValidateStatus::DialogueDoesNotContainAttendee:
	 //        FDlgLogger::Get().Errorf(
	 //            TEXT("%s - Attendee Path = `%s` with AttendeeName = `%s` is NOT referenced (DOES) not exist inside the Dialogue.\nContext:\n\tDialogue = `%s`"),
	 //            *ContextString, *Attendee->GetPathName(), *IDlgDialogAttendee::Execute_GetAttendeeName(Attendee).ToString(), *Dialogue->GetPathName()
	 //        );
		// 	return false;

		default:
			FDlgLogger::Get().Errorf(TEXT("%s - ValidateAttendeeForDialogue - Error EDlgValidateStatus Unhandled = %d"), *ContextString, static_cast<int32>(Status));
			return false;
	}
}

bool UDlgContext::ValidateAttendeesMapForDialogue(
	const FString& ContextString,
	const UDlgDialogue* Dialogue,
	const TMap<FName, UObject*>& AttendeesMap,
	bool bLog
)
{
	const FString ContextMessage = ContextString.IsEmpty()
		? FString::Printf(TEXT("ValidateAttendeesMapForDialogue"))
		: FString::Printf(TEXT("%s - ValidateAttendeesMapForDialogue"), *ContextString);

	if (!IsValid(Dialogue))
	{
		if (bLog)
		{
			FDlgLogger::Get().Errorf(TEXT("%s - FAILED because the supplied Dialogue Asset is INVALID (nullptr)"), *ContextMessage);
		}
		return false;
	}
	if (Dialogue->GetAttendeesData().Num() == 0)
	{
		if (bLog)
		{
			FDlgLogger::Get().Errorf(TEXT("%s - Dialogue = `%s` does not have any attendees"), *ContextMessage, *Dialogue->GetPathName());
		}
		return false;
	}

	// Check if at least these attendees are required
	const TMap<FName, FDlgAttendeeData>& DialogueAttendees = Dialogue->GetAttendeesData();
	TArray<FName> AttendeesRequiredArray;
	const int32 AttendeesNum = DialogueAttendees.GetKeys(AttendeesRequiredArray);
	TSet<FName> AttendeesRequiredSet{AttendeesRequiredArray};

	// Iterate over Map
	for (const auto& KeyValue : AttendeesMap)
	{
		const FName AttendeeName = KeyValue.Key;
		const UObject* Attendee = KeyValue.Value;

		// We must check this otherwise we can't get the name
		if (!ValidateAttendeeForDialogue(ContextMessage, Dialogue, Attendee, bLog))
		{
			return false;
		}

		// Check the Map Key matches the Attendee Name
		// This should only happen if you constructed the map incorrectly by mistake
		// If you used ConvertArrayOfAttendeesToMap this should have NOT happened
		{
			const FName ObjectAttendeeName = IDlgDialogAttendee::Execute_GetAttendeeName(Attendee);
			if (AttendeeName != ObjectAttendeeName)
			{
				if (bLog)
				{
					FDlgLogger::Get().Errorf(
						TEXT("%s - The Map has a KEY Attendee Name = `%s` DIFFERENT to the VALUE of the Attendee Path = `%s` with the Name = `%s` (KEY Attendee Name != VALUE Attendee Name)"),
						*ContextMessage, *AttendeeName.ToString(), *Attendee->GetPathName(), *ObjectAttendeeName.ToString()
					);
				}
				return false;
			}
		}

		// We found one attendee from our set
		if (AttendeesRequiredSet.Contains(AttendeeName))
		{
			AttendeesRequiredSet.Remove(AttendeeName);
		}
		else
		{
			// Attendee does note exist, just warn about it, we are relaxed about this
			if (bLog)
			{
				FDlgLogger::Get().Warningf(
					TEXT("%s - Attendee Path = `%s` with Attendee Name = `%s` is NOT referenced (DOES) not exist inside the Dialogue. It is going to be IGNORED.\nContext:\n\tDialogue = `%s`"),
					*ContextMessage, *Attendee->GetPathName(), *AttendeeName.ToString(), *Dialogue->GetPathName()
				);
			}
		}
	}

	// Some attendees are missing
	if (AttendeesRequiredSet.Num() > 0)
	{
		if (bLog)
		{
			TArray<FString> AttendeesMissing;
			for (const auto Name : AttendeesRequiredSet)
			{
				AttendeesMissing.Add(Name.ToString());
			}

			const FString NameList = FString::Join(AttendeesMissing, TEXT(", "));
			FDlgLogger::Get().Errorf(
				TEXT("%s - FAILED for Dialogue = `%s` because the following Attendee Names are MISSING: `%s"),
				*ContextMessage,  *Dialogue->GetPathName(), *NameList
			);
		}
		return false;
	}

	return true;
}

bool UDlgContext::ConvertArrayOfAttendeesToMap(
	const FString& ContextString,
	const UDlgDialogue* Dialogue,
	const TArray<UObject*>& AttendeesArray,
	TMap<FName, UObject*>& OutAttendeesMap,
	bool bLog
)
{
	const FString ContextMessage = ContextString.IsEmpty()
		? FString::Printf(TEXT("ConvertArrayOfAttendeesToMap"))
		: FString::Printf(TEXT("%s - ConvertArrayOfAttendeesToMap"), *ContextString);

	// We don't allow to convert empty arrays
	OutAttendeesMap.Empty();
	if (AttendeesArray.Num() == 0)
	{
		if (bLog)
		{
			FDlgLogger::Get().Errorf(
				TEXT("%s - Attendees Array is EMPTY, can't convert anything. Dialogue = `%s`"),
				*ContextMessage, Dialogue ? *Dialogue->GetPathName() : TEXT("INVALID")
			);
		}
		return false;
	}

	for (int32 Index = 0; Index < AttendeesArray.Num(); Index++)
	{
		UObject* Attendee = AttendeesArray[Index];
		const FString ContextMessageWithIndex = FString::Printf(TEXT("%s - Attendee at Index = %d"), *ContextMessage,  Index);

		// We must check this otherwise we can't get the name
		if (!ValidateAttendeeForDialogue(ContextMessageWithIndex, Dialogue, Attendee, bLog))
		{
			return false;
		}

		// Is Duplicate?
		// Just warn the user about it, but still continue our conversion
		const FName AttendeeName = IDlgDialogAttendee::Execute_GetAttendeeName(Attendee);
		if (OutAttendeesMap.Contains(AttendeeName))
		{
			if (bLog)
			{
				FDlgLogger::Get().Warningf(
					TEXT("%s - Attendee Path = `%s`, Attendee Name = `%s` already exists in the Array. Ignoring it!"),
					*ContextMessageWithIndex, *Attendee->GetPathName(), *AttendeeName.ToString()
				);
			}
			continue;
		}

		OutAttendeesMap.Add(AttendeeName, Attendee);
	}

	return true;
}
