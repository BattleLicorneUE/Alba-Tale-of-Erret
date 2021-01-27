
#include "DlgManager.h"

#include "UObject/UObjectIterator.h"
#include "Engine/ObjectLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

#include "IDlgSystemModule.h"
#include "DlgConstants.h"
#include "DlgDialogAttendee.h"
#include "DlgDialogue.h"
#include "DlgMemory.h"
#include "DlgContext.h"
#include "Logging/DlgLogger.h"
#include "DlgHelper.h"
#include "NYReflectionHelper.h"

TWeakObjectPtr<const UObject> UDlgManager::UserWorldContextObjectPtr = nullptr;

bool UDlgManager::bCalledLoadAllDialoguesIntoMemory = false;;

UDlgContext* UDlgManager::StartDialogueWithDefaultAttendees(UObject* WorldContextObject, UDlgDialogue* Dialogue)
{
	if (!IsValid(Dialogue))
	{
		FDlgLogger::Get().Error(TEXT("StartDialogueWithDefaultAttendees - FAILED to start dialogue because the Dialogue is INVALID (is nullptr)!"));
		return nullptr;
	}

	// Create empty map of attendees we need
	TSet<FName> AttendeeSet;
	Dialogue->GetAllAttendeeNames(AttendeeSet);
	TArray<UObject*> Attendees;

	// Maps from Attendee Name => Objects that have that attendee name
	TMap<FName, TArray<UObject*>> ObjectMap;
	for (const FName& Name : AttendeeSet)
	{
		ObjectMap.Add(Name, {});
	}

	// Gather all objects that have our attendee name
	for (UObject* Attendee : GetAllObjectsWithDialogueAttendeeInterface(WorldContextObject))
	{
		const FName AttendeeName = IDlgDialogAttendee::Execute_GetAttendeeName(Attendee);
		if (ObjectMap.Contains(AttendeeName))
		{
			ObjectMap[AttendeeName].AddUnique(Attendee);
			Attendees.AddUnique(Attendee);
		}
	}

	// Find the missing names and the duplicate names
	TArray<FString> MissingNames;
	TArray<FString> DuplicatedNames;
	for (const auto& Pair : ObjectMap)
	{
		const FName AttendeeName = Pair.Key;
		const TArray<UObject*>& Objects = Pair.Value;

		if (Objects.Num() == 0)
		{
			MissingNames.Add(AttendeeName.ToString());
		}
		else if (Objects.Num() > 1)
		{
			for (UObject* Obj : Objects)
			{
				DuplicatedNames.Add(Obj->GetName() + "(" + AttendeeName.ToString() + ")");
			}
		}
	}

	if (MissingNames.Num() > 0)
	{
		const FString NameList = FString::Join(MissingNames, TEXT(", "));
		FDlgLogger::Get().Errorf(
			TEXT("StartDialogueWithDefaultAttendees - FAILED Dialogue = `%s`, the system FAILED to find the following Attendee(s): %s"),
			*Dialogue->GetName(), *NameList
		);
	}

	if (DuplicatedNames.Num() > 0)
	{
		const FString NameList = FString::Join(DuplicatedNames, TEXT(", "));
		FDlgLogger::Get().Errorf(
			TEXT("StartDialogueWithDefaultAttendees - FAILED for Dialogue = `%s`, the system found multiple attendees using the same name: %s"),
			*Dialogue->GetName(), *NameList
		);
	}

	if (MissingNames.Num() > 0 || DuplicatedNames.Num() > 0)
	{
		return nullptr;
	}

	return StartDialogueWithContext(TEXT("StartDialogueWithDefaultAttendees"), Dialogue, Attendees);
}

UDlgContext* UDlgManager::StartDialogueWithContext(const FString& ContextString, UDlgDialogue* Dialogue, const TArray<UObject*>& Attendees)
{
	const FString ContextMessage = ContextString.IsEmpty()
		? FString::Printf(TEXT("StartDialogue"))
		: FString::Printf(TEXT("%s - StartDialogue"), *ContextString);

	TMap<FName, UObject*> AttendeeBinding;
	if (!UDlgContext::ConvertArrayOfAttendeesToMap(ContextMessage, Dialogue, Attendees, AttendeeBinding))
	{
		return nullptr;
	}

	auto* Context = NewObject<UDlgContext>(Attendees[0], UDlgContext::StaticClass());
	if (Context->StartWithContext(ContextMessage, Dialogue, AttendeeBinding))
	{
		return Context;
	}

	return nullptr;
}

bool UDlgManager::CanStartDialogue(UDlgDialogue* Dialogue, UPARAM(ref)const TArray<UObject*>& Attendees)
{
	TMap<FName, UObject*> AttendeeBinding;
	if (!UDlgContext::ConvertArrayOfAttendeesToMap(TEXT("CanStartDialogue"), Dialogue, Attendees, AttendeeBinding, false))
	{
		return false;
	}

	return UDlgContext::CanBeStarted(Dialogue, AttendeeBinding);
}

UDlgContext* UDlgManager::ResumeDialogueFromNodeIndex(
	UDlgDialogue* Dialogue,
	UPARAM(ref)const TArray<UObject*>& Attendees,
	int32 StartNodeIndex,
	const TSet<int32>& AlreadyVisitedNodes,
	bool bFireEnterEvents
)
{
	const FString ContextMessage = TEXT("ResumeDialogueFromNodeIndex");
	TMap<FName, UObject*> AttendeeBinding;
	if (!UDlgContext::ConvertArrayOfAttendeesToMap(ContextMessage, Dialogue, Attendees, AttendeeBinding))
	{
		return nullptr;
	}

	auto* Context = NewObject<UDlgContext>(Attendees[0], UDlgContext::StaticClass());
	FDlgHistory History;
	History.VisitedNodeIndices = AlreadyVisitedNodes;
	if (Context->StartWithContextFromNodeIndex(ContextMessage, Dialogue, AttendeeBinding, StartNodeIndex, History, bFireEnterEvents))
	{
		return Context;
	} 

	return nullptr;
}

UDlgContext* UDlgManager::ResumeDialogueFromNodeGUID(
	UDlgDialogue* Dialogue,
	UPARAM(ref)const TArray<UObject*>& Attendees,
	const FGuid& StartNodeGUID,
	const TSet<FGuid>& AlreadyVisitedNodes,
	bool bFireEnterEvents
)
{
	const FString ContextMessage = TEXT("ResumeDialogueFromNodeGUID");
	TMap<FName, UObject*> AttendeeBinding;
	if (!UDlgContext::ConvertArrayOfAttendeesToMap(ContextMessage, Dialogue, Attendees, AttendeeBinding))
	{
		return nullptr;
	}

	auto* Context = NewObject<UDlgContext>(Attendees[0], UDlgContext::StaticClass());
	FDlgHistory History;
	History.VisitedNodeGUIDs = AlreadyVisitedNodes;
	if (Context->StartWithContextFromNodeGUID(ContextMessage, Dialogue, AttendeeBinding, StartNodeGUID, History, bFireEnterEvents))
	{
		return Context;
	}

	return nullptr;
}

UDlgContext* UDlgManager::StartMonologue(UDlgDialogue* Dialogue, UObject* Attendee)
{
	TArray<UObject*> Attendees;
	Attendees.Add(Attendee);
	return StartDialogueWithContext(TEXT("StartMonologue"), Dialogue, Attendees);
}

UDlgContext* UDlgManager::StartDialogue2(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1)
{
	TArray<UObject*> Attendees;
	Attendees.Add(Attendee0);
	Attendees.Add(Attendee1);
	return StartDialogueWithContext(TEXT("StartDialogue2"), Dialogue, Attendees);
}

UDlgContext* UDlgManager::StartDialogue3(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1, UObject* Attendee2)
{
	TArray<UObject*> Attendees;
	Attendees.Add(Attendee0);
	Attendees.Add(Attendee1);
	Attendees.Add(Attendee2);
	return StartDialogueWithContext(TEXT("StartDialogue3"), Dialogue, Attendees);
}

UDlgContext* UDlgManager::StartDialogue4(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1, UObject* Attendee2, UObject* Attendee3)
{
	TArray<UObject*> Attendees;
	Attendees.Add(Attendee0);
	Attendees.Add(Attendee1);
	Attendees.Add(Attendee2);
	Attendees.Add(Attendee3);

	return StartDialogueWithContext(TEXT("StartDialogue4"), Dialogue, Attendees);
}

int32 UDlgManager::LoadAllDialoguesIntoMemory(bool bAsync)
{
	bCalledLoadAllDialoguesIntoMemory = true;

	// NOTE: All paths must NOT have the forward slash "/" at the end.
	// If they do, then this won't load Dialogues that are located in the Content root directory
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(UDlgDialogue::StaticClass(), false, GIsEditor);
	TArray<FString> PathsToSearch = { TEXT("/Game") };
	ObjectLibrary->AddToRoot();

	// Add the current plugin dir
	// TODO maybe add all the non engine plugin paths? IPluginManager::Get().GetEnabledPlugins()
	const TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(DIALOGUE_SYSTEM_PLUGIN_NAME.ToString());
	if (ThisPlugin.IsValid())
	{
		FString PluginPath = ThisPlugin->GetMountedAssetPath();
		// See NOTE above
		PluginPath.RemoveFromEnd(TEXT("/"));
		PathsToSearch.Add(PluginPath);
	}

	const bool bForceSynchronousScan = !bAsync;
	const int32 Count = ObjectLibrary->LoadAssetDataFromPaths(PathsToSearch, bForceSynchronousScan);
	ObjectLibrary->LoadAssetsFromAssetData();
	ObjectLibrary->RemoveFromRoot();

	return Count;
}

TArray<UDlgDialogue*> UDlgManager::GetAllDialoguesFromMemory()
{
#if WITH_EDITOR
	// Hmm, something is wrong
	if (!bCalledLoadAllDialoguesIntoMemory)
	{
		LoadAllDialoguesIntoMemory(false);
	}
// 	check(bCalledLoadAllDialoguesIntoMemory);
#endif

	TArray<UDlgDialogue*> Array;
	for (TObjectIterator<UDlgDialogue> Itr; Itr; ++Itr)
	{
		UDlgDialogue* Dialogue = *Itr;
		if (IsValid(Dialogue))
		{
			Array.Add(Dialogue);
		}
	}
	return Array;
}

TArray<TWeakObjectPtr<AActor>> UDlgManager::GetAllWeakActorsWithDialogueAttendeeInterface(UWorld* World)
{
	TArray<TWeakObjectPtr<AActor>> Array;
	for (TActorIterator<AActor> Itr(World); Itr; ++Itr)
	{
		AActor* Actor = *Itr;
		if (IsValid(Actor) && !Actor->IsPendingKill() && Actor->GetClass()->ImplementsInterface(UDlgDialogAttendee::StaticClass()))
		{
			Array.Add(Actor);
		}
	}
	return Array;
}

TArray<UObject*> UDlgManager::GetAllObjectsWithDialogueAttendeeInterface(UObject* WorldContextObject)
{
	TArray<UObject*> Array;
	if (!WorldContextObject)
		return Array;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		// TObjectIterator has some weird ghost objects in editor, I failed to find a way to validate them
		// Instead of this ActorIterate is used and the properties inside the actors are examined in a recursive way
		// Containers are not supported yet
		TSet<UObject*> VisitedSet;
		for (TActorIterator<AActor> Itr(World); Itr; ++Itr)
		{
			GatherAttendeesRecursive(*Itr, Array, VisitedSet);
		}
	}

	// TArray<UObject*> Array2;
	// for (TObjectIterator<UObject> Itr; Itr; ++Itr)
	// {
	// 	UObject* Object = *Itr;
	// 	if (IsValid(Object)
	// 		&& !Object->IsPendingKill()
	// 		&& Object->GetClass()->ImplementsInterface(UDlgDialogAttendee::StaticClass()))
	// 		//&& IDlgDialogAttendee::Execute_GetAttendeeName(Object) != NAME_None)
	// 	{
	// 		if (Object->HasAllFlags(RF_Transient | RF_Transactional) || !Object->HasAnyFlags(RF_Transient) )
	// 		{
	// 			Array2.AddUnique(Object);
	// 		}
	// 	}
	// }

	return Array;
}

TMap<FName, FDlgObjectsArray> UDlgManager::GetAllObjectsMapWithDialogueAttendeeInterface(UObject* WorldContextObject)
{
	// Maps from Attendee Name => Objects that have that attendee name
	TMap<FName, FDlgObjectsArray> ObjectsMap;
	for (UObject* Attendee : GetAllObjectsWithDialogueAttendeeInterface(WorldContextObject))
	{
		const FName AttendeeName = IDlgDialogAttendee::Execute_GetAttendeeName(Attendee);
		if (ObjectsMap.Contains(AttendeeName))
		{
			// Update
			ObjectsMap[AttendeeName].Array.Add(Attendee);
		}
		else
		{
			// Create
			FDlgObjectsArray ArrayStruct;
			ArrayStruct.Array.Add(Attendee);
			ObjectsMap.Add(AttendeeName, ArrayStruct);
		}
	}

	return ObjectsMap;
}

TArray<UDlgDialogue*> UDlgManager::GetDialoguesWithDuplicateGUIDs()
{
	TArray<UDlgDialogue*> Dialogues = GetAllDialoguesFromMemory();
	TArray<UDlgDialogue*> DuplicateDialogues;

	TSet<FGuid> DialogueGUIDs;
	for (UDlgDialogue* Dialogue : Dialogues)
	{
		const FGuid ID = Dialogue->GetGUID();
		if (DialogueGUIDs.Find(ID) == nullptr)
		{
			// does not exist, good
			DialogueGUIDs.Add(ID);
		}
		else
		{
			// how?
			DuplicateDialogues.Add(Dialogue);
		}
	}

	return DuplicateDialogues;
}

TMap<FGuid, UDlgDialogue*> UDlgManager::GetAllDialoguesGUIDsMap()
{
	TArray<UDlgDialogue*> Dialogues = GetAllDialoguesFromMemory();
	TMap<FGuid, UDlgDialogue*> DialoguesMap;

	for (UDlgDialogue* Dialogue : Dialogues)
	{
		const FGuid ID = Dialogue->GetGUID();
		if (DialoguesMap.Contains(ID))
		{
			FDlgLogger::Get().Errorf(
				TEXT("GetAllDialoguesGUIDsMap - ID = `%s` for Dialogue = `%s` already exists"),
				*ID.ToString(), *Dialogue->GetPathName()
			);
		}

		DialoguesMap.Add(ID, Dialogue);
	}

	return DialoguesMap;
}

const TMap<FGuid, FDlgHistory>& UDlgManager::GetDialogueHistory()
{
	return FDlgMemory::Get().GetHistoryMaps();
}

void UDlgManager::SetDialogueHistory(const TMap<FGuid, FDlgHistory>& DlgHistory)
{
	FDlgMemory::Get().SetHistoryMap(DlgHistory);
}

void UDlgManager::ClearDialogueHistory()
{
	FDlgMemory::Get().Empty();
}

bool UDlgManager::DoesObjectImplementDialogueAttendeeInterface(const UObject* Object)
{
	return FDlgHelper::IsObjectImplementingInterface(Object, UDlgDialogAttendee::StaticClass());
}

bool UDlgManager::IsObjectACustomEvent(const UObject* Object)
{
	return FDlgHelper::IsObjectAChildOf(Object, UDlgEventCustom::StaticClass());
}

bool UDlgManager::IsObjectACustomCondition(const UObject* Object)
{
	return FDlgHelper::IsObjectAChildOf(Object, UDlgConditionCustom::StaticClass());
}

bool UDlgManager::IsObjectACustomTextArgument(const UObject* Object)
{
	return FDlgHelper::IsObjectAChildOf(Object, UDlgTextArgumentCustom::StaticClass());
}

bool UDlgManager::IsObjectANodeData(const UObject* Object)
{
	return FDlgHelper::IsObjectAChildOf(Object, UDlgNodeData::StaticClass());
}

TArray<UDlgDialogue*> UDlgManager::GetAllDialoguesForAttendeeName(FName AttendeeName)
{
	TArray<UDlgDialogue*> DialoguesArray;
	for (UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		if (Dialogue->HasAttendee(AttendeeName))
		{
			DialoguesArray.Add(Dialogue);
		}
	}

	return DialoguesArray;
}

void UDlgManager::GetAllDialoguesAttendeeNames(TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetAllAttendeeNames(UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesSpeakerStates(TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{ 
		Dialogue->GetAllSpeakerStates(UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesIntNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetIntNames(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesFloatNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetFloatNames(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesBoolNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetBoolNames(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesNameNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetNameNames(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesConditionNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetConditions(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

void UDlgManager::GetAllDialoguesEventNames(FName AttendeeName, TArray<FName>& OutArray)
{
	TSet<FName> UniqueNames;
	for (const UDlgDialogue* Dialogue : GetAllDialoguesFromMemory())
	{
		Dialogue->GetEvents(AttendeeName, UniqueNames);
	}

	FDlgHelper::AppendSortedSetToArray(UniqueNames, OutArray);
}

bool UDlgManager::RegisterDialogueConsoleCommands()
{
	if (!IDlgSystemModule::IsAvailable())
	{
		FDlgLogger::Get().Error(TEXT("RegisterDialogueConsoleCommands - The Dialogue System Module is NOT Loaded"));
		return false;
	}

	IDlgSystemModule::Get().RegisterConsoleCommands(GetDialogueWorld());
	return true;
}

bool UDlgManager::UnregisterDialogueConsoleCommands()
{
	if (!IDlgSystemModule::IsAvailable())
	{
		FDlgLogger::Get().Error(TEXT("UnregisterDialogueConsoleCommands - The Dialogue System Module is NOT Loaded"));
		return false;
	}

	IDlgSystemModule::Get().UnregisterConsoleCommands();
	return true;
}

void UDlgManager::GatherAttendeesRecursive(UObject* Object, TArray<UObject*>& Array, TSet<UObject*>& AlreadyVisited)
{
	if (!IsValid(Object) || Object->IsPendingKill() || AlreadyVisited.Contains(Object))
	{
		return;
	}

	AlreadyVisited.Add(Object);
	if (Object->GetClass()->ImplementsInterface(UDlgDialogAttendee::StaticClass()))
	{
		Array.Add(Object);
	}

	// Gather recursive from children
	for (auto* Property = Object->GetClass()->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
	{
		if (auto* ObjectProperty = FNYReflectionHelper::CastProperty<FNYObjectProperty>(Property))
		{
			GatherAttendeesRecursive(ObjectProperty->GetPropertyValue_InContainer(Object), Array, AlreadyVisited);
		}

		// TODO: handle containers and structs
	}
}

UWorld* UDlgManager::GetDialogueWorld()
{
	// Try to use the user set one
	if (UserWorldContextObjectPtr.IsValid())
	{
		if (auto* WorldContextObject = UserWorldContextObjectPtr.Get())
		{
			if (auto* World = WorldContextObject->GetWorld())
			{
				return World;
			}
		}
	}

	// Fallback to default autodetection
	if (GEngine)
	{
		// Get first PIE world
		// Copied from TakeUtils::GetFirstPIEWorld()
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsPlayInEditor())
				continue;

			if (World->GetNetMode() == ENetMode::NM_Standalone ||
				(World->GetNetMode() == ENetMode::NM_Client && Context.PIEInstance == 2))
			{
				return World;
			}
		}

		// Otherwise get the first Game World
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
				continue;

			return World;
		}
	}

	FDlgLogger::Get().Error(TEXT("GetDialogueWorld - Could NOT find any valid world. Call SetDialoguePersistentWorldContextObject in the Being Play of your GameMode"));
	return nullptr;
}
