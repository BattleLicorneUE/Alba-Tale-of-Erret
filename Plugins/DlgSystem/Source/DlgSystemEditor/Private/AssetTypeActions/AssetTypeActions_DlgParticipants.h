
#pragma once

#include "AssetTypeActions_Base.h"
#include "DlgDialogAttendee.h"

class IToolkitHost;

/**
 * See FDlgSystemEditorModule::StartupModule for usage.
 * Search for all objects
 */
// class FAssetTypeActions_DlgAttendees : public FAssetTypeActions_Base
// {
// public:
// 	//
// 	// IAssetTypeActions interface
// 	//
// 	FAssetTypeActions_DlgAttendees(EAssetTypeCategories::Type InAssetCategory) : AssetCategory(InAssetCategory) {}
//
// 	FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "DlgAttendeeAssetTypeActions", "Dialogue Attendee"); }
// 	UClass* GetSupportedClass() const override { return UObject::StaticClass(); }
// 	FColor GetTypeColor() const override { return FColor(232, 232, 0); }
// 	bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
// 	uint32 GetCategories() override { return EAssetTypeCategories::Blueprint; }
// 	bool CanFilter() override { return true; }
// 	void BuildBackendFilter(FARFilter& InFilter) override
// 	{
// 		// ImplementedInterfaces is an array of structs (FBPInterfaceDescription). When exported to an AR tag value, each entry will be formatted as:
// 		//
// 		//	Entry := (Interface=Type'Package.Class')
// 		// The full tag value (array of exported struct values) will then be formatted as follows:
// 		//
// 		//  Value := (Entry1,Entry2,...EntryN)
// 		const FString Value = FString::Printf(
// 			TEXT("%s"),
// 			*UDlgDialogAttendee::StaticClass()->GetPathName()
// 		);
// 		//ImplementedInterfaces=/Script/DlgSystem.DlgDialogAttendee
// 		//ImplementedInterfaces=/Script/DlgSystem.DlgDialogAttendee
// 		InFilter.TagsAndValues.Add(FBlueprintTags::ImplementedInterfaces, Value);
//
// 		FBPInterfaceDescription Interface;
//
// 		InFilter.ClassNames.Add(GetSupportedClass()->GetFName());
// 		InFilter.bRecursiveClasses = true;
// 	}
//
// protected:
// 	EAssetTypeCategories::Type AssetCategory;
// };
