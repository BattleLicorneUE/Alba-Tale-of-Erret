
#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DlgDialogue.h"
#include "DlgDialogAttendee.h"
#include "DlgMemory.h"

#include "DlgManager.generated.h"

class AActor;
class UDlgContext;
class UDlgDialogue;


USTRUCT(BlueprintType)
struct DLGSYSTEM_API FDlgObjectsArray
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<UObject*> Array;
};

/**
 *  Class providing a collection of static functions to start a conversation and work with Dialogues.
 */
UCLASS()
class DLGSYSTEM_API UDlgManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Starts a Dialogue with the provided Dialogue
	 * The function checks all the objects in the world to gather the attendees
	 * This method can fail in the following situations:
	 *  - The Dialogue has a Attendee which does not exist in the World
	 *	- Multiple Objects are using the same Attendee Name in the World
	 *
	 *	NOTE: If this fails because it can't find the unique attendees you should use the StartDialogue* functions
	 *
	 * @returns The dialogue context object or nullptr if something went wrong
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch", meta = (WorldContext = "WorldContextObject"))
	static UDlgContext* StartDialogueWithDefaultAttendees(UObject* WorldContextObject, UDlgDialogue* Dialogue);

	// Supplies where we called this from
	static UDlgContext* StartDialogueWithContext(const FString& ContextString, UDlgDialogue* Dialogue, const TArray<UObject*>& Attendees);

	/**
	 * Starts a Dialogue with the provided Dialogue and Attendees array
	 * This method can fail in the following situations:
	 *  - The Attendees number does not match the number of attendees from the Dialogue.
	 *  - Any UObject in the Attendee array does not implement the Attendee Interface
	 *  - Attendee->GetAttendeeName() does not exist in the Dialogue
	 *
	 * @returns The dialogue context object or nullptr if something wrong happened
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* StartDialogue(UDlgDialogue* Dialogue, UPARAM(ref)const TArray<UObject*>& Attendees)
	{
		return StartDialogueWithContext(TEXT("StartDialogue"), Dialogue, Attendees);
	}

	/**
	 * Checks if there is any child of the start node which can be enterred based on the conditions
	 *
	 * @returns true if there is an enterable node from the start node
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static bool CanStartDialogue(UDlgDialogue* Dialogue, UPARAM(ref)const TArray<UObject*>& Attendees);

	/**
	 * Starts a Dialogue with the provided Dialogue and Attendees array, at the given entry point
	 *
	 * NOTE: You should most likely use ResumeDialogueFromNodeGUID
	 *
	 * This method can fail in the following situations:
	 *  - The Attendees number does not match the number of attendees from the Dialogue.
	 *  - Any UObject in the Attendee array does not implement the Attendee Interface
	 *  - Attendee->GetAttendeeName() does not exist in the Dialogue
	 *  - The given node index is invalid
	 *  - The starter node does not have any valid child
	 *
	 * @param Dialogue				- The dialogue asset to start
	 * @param Attendees			- Array of attendees, has to match with the expected input for the Dialogue
	 * @param StartIndex		    - Index of the node the dialogue is resumed at
	 * @param AlreadyVisitedNodes	- Set of nodes already visited in the context the last time this Dialogue was going on.
	 *								  Can be acquired via GetVisitedNodeIndices() on the context
	 * @param bFireEnterEvents		- decides if the enter events should be fired on the resumed node or not
	 * @returns The dialogue context object or nullptr if something wrong happened
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* ResumeDialogueFromNodeIndex(
		UDlgDialogue* Dialogue,
		UPARAM(ref)const TArray<UObject*>& Attendees,
		UPARAM(DisplayName="Start Node Index") int32 StartIndex,
		const TSet<int32>& AlreadyVisitedNodes,
		bool bFireEnterEvents
	);

	/**
	* Starts a Dialogue with the provided Dialogue and Attendees array, at the given entry point
	*
	* This method can fail in the following situations:
	*  - The Attendees number does not match the number of attendees from the Dialogue.
	*  - Any UObject in the Attendee array does not implement the Attendee Interface
	*  - Attendee->GetAttendeeName() does not exist in the Dialogue
	*  - The given node GUID is invalid
	*  - The starter node does not have any valid child
	*
	* @param Dialogue				- The dialogue asset to start
	* @param Attendees			- Array of attendees, has to match with the expected input for the Dialogue
	* @param StartNodeGUID			- GUID of the node the dialogue is resumed at
	* @param AlreadyVisitedNodes	- Set of nodes already visited in the context the last time this Dialogue was going on.
	*								  Can be acquired via GetVisitedNodeGUIDs() on the context
	* @param bFireEnterEvents		- decides if the enter events should be fired on the resumed node or not
	* @returns The dialogue context object or nullptr if something wrong happened
	*/
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
    static UDlgContext* ResumeDialogueFromNodeGUID(
        UDlgDialogue* Dialogue,
        UPARAM(ref)const TArray<UObject*>& Attendees,
        const FGuid& StartNodeGUID,
        const TSet<FGuid>& AlreadyVisitedNodes,
        bool bFireEnterEvents
    );


	//
	// Helper methods, same as StartDialogue but with fixed amount of attendee(s)
	//

	// Helper methods that allows you to start a Dialogue with only a attendee
	// For N Attendees just use StartDialogue
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* StartMonologue(UDlgDialogue* Dialogue, UObject* Attendee);

	// Helper methods that allows you to start a Dialogue with 2 attendees
	// For N Attendees just use StartDialogue
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* StartDialogue2(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1);

	// Helper methods that allows you to start a Dialogue with 3 attendees
	// For N Attendees just use StartDialogue
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* StartDialogue3(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1, UObject* Attendee2);

	// Helper methods that allows you to start a Dialogue with 4 attendees
	// For N Attendees just use StartDialogue
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Launch")
	static UDlgContext* StartDialogue4(UDlgDialogue* Dialogue, UObject* Attendee0, UObject* Attendee1, UObject* Attendee2, UObject* Attendee3);

	/**
	 * Loads all dialogues from the filesystem into memory
	 * @return number of loaded dialogues
	 */
	static int32 LoadAllDialoguesIntoMemory(bool bAsync = false);

	// Gets all loaded dialogues from memory. LoadAllDialoguesIntoMemory must be called before this
	static TArray<UDlgDialogue*> GetAllDialoguesFromMemory();

	// Gets all the objects from the provided World that implement the Dialogue Attendee Interface. Iterates through all objects, DO NOT CALL EACH FRAME
	static TArray<TWeakObjectPtr<AActor>> GetAllWeakActorsWithDialogueAttendeeInterface(UWorld* World);

	// Gets all objects from the World that implement the Dialogue Attendee Interface
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", meta = (WorldContext = "WorldContextObject"))
	static TArray<UObject*> GetAllObjectsWithDialogueAttendeeInterface(UObject* WorldContextObject);

	// Same as GetAllObjectsWithDialogueAttendeeInterface but groups the Objects into a Map
	// Where the Key is the Attendee Name
	// and the Value is the Attendees Array
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", meta = (WorldContext = "WorldContextObject"))
	static TMap<FName, FDlgObjectsArray> GetAllObjectsMapWithDialogueAttendeeInterface(UObject* WorldContextObject);

	// Gets all the dialogues that have a duplicate GUID, should not happen, like ever.
	static TArray<UDlgDialogue*> GetDialoguesWithDuplicateGUIDs();

	// Helper methods that gets all the dialogues in a map by guid.
	static TMap<FGuid, UDlgDialogue*> GetAllDialoguesGUIDsMap();

	// Gets all the loaded dialogues from memory that have the AttendeeName included inside them.
	static TArray<UDlgDialogue*> GetAllDialoguesForAttendeeName(FName AttendeeName);

	// Sets the FDlgMemory Dialogue history.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Memory")
	static void SetDialogueHistory(const TMap<FGuid, FDlgHistory>& DlgHistory);

	// Empties the FDlgMemory Dialogue history.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Memory")
	static void ClearDialogueHistory();

	// Gets the Dialogue History from the FDlgMemory.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Memory")
	static const TMap<FGuid, FDlgHistory>& GetDialogueHistory();

	// Does the Object implement the Dialogue Attendee Interface?
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper")
	static bool DoesObjectImplementDialogueAttendeeInterface(const UObject* Object);

	// Is Object a UDlgEventCustom or a child from that
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", DisplayName = "Is Object A Custom Event")
	static bool IsObjectACustomEvent(const UObject* Object);

	// Is Object a UDlgConditionCustom or a child from that
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", DisplayName = "Is Object A Custom Condition")
	static bool IsObjectACustomCondition(const UObject* Object);

	// Is Object a UDlgTextArgumentCustom or a child from that
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", DisplayName = "Is Object A Custom Text Argument")
	static bool IsObjectACustomTextArgument(const UObject* Object);

	// Is Object a UDlgNodeData or a child from that
	UFUNCTION(BlueprintPure, Category = "Dialogue|Helper", DisplayName = "Is Object A Node Data")
    static bool IsObjectANodeData(const UObject* Object);

	// Gets all the unique attendee names sorted alphabetically from all the Dialogues loaded into memory.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesAttendeeNames(TArray<FName>& OutArray);

	// Gets all the used speaker states sorted alphabetically from all the Dialogues loaded into memory.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesSpeakerStates(TArray<FName>& OutArray);

	// Gets all the unique int variable names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesIntNames(FName AttendeeName, TArray<FName>& OutArray);

	// Gets all the unique float variable names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesFloatNames(FName AttendeeName, TArray<FName>& OutArray);

	// Gets all the unique bool variable names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesBoolNames(FName AttendeeName, TArray<FName>& OutArray);

	// Gets all the unique name variable names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesNameNames(FName AttendeeName, TArray<FName>& OutArray);

	// Gets all the unique condition names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesConditionNames(FName AttendeeName, TArray<FName>& OutArray);

	// Gets all the unique event names sorted alphabetically for the specified AttendeeName from the loaded Dialogues
	UFUNCTION(BlueprintPure, Category = "Dialogue|Data")
	static void GetAllDialoguesEventNames(FName AttendeeName, TArray<FName>& OutArray);

	// Registers all the DlgSystem Module console commands.
	// To set the custom reference WorldContextObjectPtr, set it with SetDialoguePersistentWorldContextObject
	// @return true on success, false otherwise
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Console")
	static bool RegisterDialogueConsoleCommands();

	// Unregister all the DlgSystem Module console commands.
	// @return true on success, false otherwise
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Console")
	static bool UnregisterDialogueConsoleCommands();


	// This tries to get the source world for the dialogues
	// In the following order (the first one that is valid, returns that):
	// 1. The user set one UserWorldContextObjectPtr (if it is set):
	//    - Set  with SetDialoguePersistentWorldContextObject
	//    - Clear with ClearDialoguePersistentWorldContextObject
	// 2. The first PIE world
	// 3. The first Game World
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Persistence")
	static UWorld* GetDialogueWorld();

	// If the user wants to set the world context object manually
	// Otherwise just use GetDialogueWorld
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Persistence")
	static void SetDialoguePersistentWorldContextObject(const UObject* WorldContextObject)
	{
		UserWorldContextObjectPtr = WorldContextObject;
	}

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Persistence")
	static void ClearDialoguePersistentWorldContextObject()
	{
		UserWorldContextObjectPtr.Reset();
	}

private:
	static void GatherAttendeesRecursive(UObject* Object, TArray<UObject*>& Array, TSet<UObject*>& AlreadyVisited);

	// Set by the user, we will default to automagically resolve the world
	static TWeakObjectPtr<const UObject> UserWorldContextObjectPtr;

	static bool bCalledLoadAllDialoguesIntoMemory;
};
