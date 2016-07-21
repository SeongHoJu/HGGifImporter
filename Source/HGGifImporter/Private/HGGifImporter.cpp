// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HGGifImporterPrivatePCH.h"


class FHGGifImporter : public IHGGifImporter
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FHGGifImporter, HGGifImporter )



void FHGGifImporter::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FHGGifImporter::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



