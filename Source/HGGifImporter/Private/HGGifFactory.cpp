// Fill out your copyright notice in the Description page of Project Settings.


#include "HGGifImporterPrivatePCH.h"

#include "ContentBrowserModule.h"
#include "AssetToolsModule.h"
#include "Factories/Factory.h"
#include "PaperImporterSettings.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookFactory.h"
#include "PaperFlipbookHelpers.h"


#include "HGGifFactory.h"

DEFINE_LOG_CATEGORY_STATIC(LogEditorFactories, Log, All);

TArray<FBinaryImage> UHGGifFactory::CachedBinaryImages;

UHGGifFactory::UHGGifFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UPaperFlipbook::StaticClass();

	Formats.Empty();
	Formats.Add(TEXT("gif;Flipbook"));

	if (GenerateConverterDll() == false)
	{
		UE_LOG(LogEditorFactories, Log, TEXT("Load Gifconverter.dll is Failed"));
	}
}


UObject* UHGGifFactory::FactoryCreateBinary
(
	UClass*				Class,
	UObject*			InParent,
	FName				Name,
	EObjectFlags		Flags,
	UObject*			Context,
	const TCHAR*		Type,
	const uint8*&		Buffer,
	const uint8*		BufferEnd,
	FFeedbackContext*	Warn
	)
{
	check(Type);

	// 1. Copy Gif File to Buffer
	const int32 Length = BufferEnd - Buffer;
	TArray<uint8> GifBytes;
	GifBytes.AddUninitialized(Length);	// Allocate Empty Space
	
	FMemory::Memcpy(GifBytes.GetData(), Buffer, Length);

	// 2. Decode Gif File using Gifconverter.dll
	TArray<FBinaryImage> BinaryImages;
	DecodeGifFile(GifBytes, BinaryImages);

	// 3. Convert to Texture2D from Decoded Binary Images
	TArray<UTexture2D*> GifFrameTextures;
	GifFrameTextures.Empty(BinaryImages.Num());
	GenerateTextures(BinaryImages, GifFrameTextures, InParent, Name, Type, Flags, Context, Warn);

	// 4. Create PaperSprite From GifFrame Texture2D
	TArray<UPaperSprite*> Sprites;
	Sprites.Empty(GifFrameTextures.Num());
	GenerateSprites(GifFrameTextures, Sprites, InParent, Flags, Context, Warn);

	// 5. Create PaperBook
	UPaperFlipbook* NewFlipbook = CreateFlipBook(Sprites, Name);

	// 6. Free Allocated Memory
	CleanUpFactory(BinaryImages);

	return NewFlipbook;
}


bool UHGGifFactory::DecodeGifFile(const TArray<uint8>& GifBytes, TArray<FBinaryImage>& GifFrames)
{
	if (GeneratedDllHandle != nullptr)
	{
		/*
		@Param : GifBinaries - Gif File Binary Address
		@Param : BinarySize - Gif File Size
		@Param : _CallbackRequestBitmapAlloc - Callback function After
		*/
		if (GifDecodeFunction((void*)GifBytes.GetData(), (int32)GifBytes.Num(), CallbackRequestAlloc))
		{
			// Copy Cached Image - Just Pointer, int64
			GifFrames = UHGGifFactory::CachedBinaryImages;

			// Keep Maximum Heap Allocation;
			UHGGifFactory::CachedBinaryImages.Reset(CachedBinaryImages.GetSlack());
			return true;
		}
	}

	return false;
}

/*
Called by C# Dll( GifConverter.dll )

after GifBinary is Decoded, C# dll call this function each gif frameimage

*/
void* UHGGifFactory::CallbackRequestHeapAlloc(int32 FrameIndex, int64 ImageSize)
{
	/*
	Param@ FrameIndex : Decoded Gif Image Frame
	Param@ ImageSize : Decoded Gif Image Size
	*/

	if (ImageSize > 0)
	{
		void* AllocMemory = FMemory::Malloc(ImageSize);
		if (static_cast<uint8*>(AllocMemory) != nullptr)
		{
			FBinaryImage NewBinary;
			NewBinary.AllocAddress = static_cast<uint8*>(AllocMemory);
			NewBinary.ImageSize = ImageSize;

			UHGGifFactory::CachedBinaryImages.Add(NewBinary);

			return AllocMemory;
		}
	}

	return nullptr;
}

/*
	Get Dll Handle & C# Function Address
*/
bool UHGGifFactory::GenerateConverterDll()
{
	if (GeneratedDllHandle == nullptr)
	{
		FString DllPath = FPaths::Combine(*FPaths::EnginePluginsDir(), TEXT("Editor/HGGifImporter/Binaries/Win64/"), TEXT("GifConverter.dll"));
		if (FPaths::FileExists(DllPath))
		{
			GeneratedDllHandle = FPlatformProcess::GetDllHandle(*DllPath); // Retrieve the DLL.
			if (GeneratedDllHandle != nullptr)
			{
				static FString procName = TEXT("GetGifInfo"); // The exact name of the DLL function.
				GifDecodeFunction = (_DecodeFunction)FPlatformProcess::GetDllExport(GeneratedDllHandle, *procName); // Export the DLL function.
				if (GifDecodeFunction != nullptr)
				{
					CallbackRequestAlloc = UHGGifFactory::CallbackRequestHeapAlloc;
					return true;
				}
			}
		}
	}

	return false;
}



bool UHGGifFactory::GenerateTextures(const TArray<FBinaryImage>& BinaryImages, /*out*/ TArray<UTexture2D*>& OutTextures, UObject* InParent, FName Name, const TCHAR* Type, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	FString TextureBaseName = Name.ToString();
	int32 TextureIndex = 0;
	for (const FBinaryImage& BinaryImage : BinaryImages)
	{
		const uint8* Ptr = const_cast<uint8*>(BinaryImage.AllocAddress);
		FString NewTextureName = FString::Printf(TEXT("%s_%d"), *TextureBaseName, TextureIndex++);

		UTexture2D* FrameTexture = Cast<UTexture2D>(Super::FactoryCreateBinary(UTexture2D::StaticClass(), InParent, *NewTextureName, Flags, Context, Type, Ptr, Ptr + BinaryImage.ImageSize, Warn));
		if (FrameTexture)
		{
			OutTextures.Add(FrameTexture);
		}
	}

	return (BinaryImages.Num() == OutTextures.Num()) ? true : false;
}


bool UHGGifFactory::GenerateSprites(const TArray<UTexture2D*>& Textures, /*out*/TArray<UPaperSprite*>& OutSprites, UObject* InParent, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<UObject*> ObjectsToSync;

	for (UTexture2D* Texture : Textures)
	{
		if (Texture != nullptr)
		{
			FString Name;
			FString PackageName;

			const FString DefaultSuffix = TEXT("_Sprite");
			AssetToolsModule.Get().CreateUniqueAssetName(Texture->GetName(), DefaultSuffix, /*out*/ PackageName, /*out*/ Name);

			UPaperSprite* NewAsset = CreateNewSprite(Texture, UPaperSprite::StaticClass(), InParent, *Name, Flags, Context, Warn);
			ObjectsToSync.Add(NewAsset);
			OutSprites.Add(NewAsset);
		}
	}

	if (ObjectsToSync.Num() > 0)
	{
		ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
		return true;
	}

	return false;
}


UPaperSprite* UHGGifFactory::CreateNewSprite(UTexture2D* InitialTexture, UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPaperSprite* NewSprite = NewObject<UPaperSprite>(InParent, Class, Name, Flags | RF_Transactional);
	if (NewSprite && InitialTexture)
	{
		FSpriteAssetInitParameters SpriteInitParams;
		SpriteInitParams.SetTextureAndFill(InitialTexture);

		const UPaperImporterSettings* ImporterSettings = GetDefault<UPaperImporterSettings>();

		ImporterSettings->ApplySettingsForSpriteInit(SpriteInitParams, ESpriteInitMaterialLightingMode::Automatic);
		NewSprite->InitializeSprite(SpriteInitParams);
	}

	return NewSprite;
}



UPaperFlipbook* UHGGifFactory::CreateFlipBook(const TArray<UPaperSprite*>& AllSprites, FName BaseName)
{
	if (AllSprites.Num() == 0)
		return nullptr;

	GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "Paper2D_CreateFlipbooks", "Creating flipbooks from selection"), true, true);

	const FString& FlipbookName = BaseName.ToString();
	TArray<UPaperSprite*> Sprites = AllSprites;	// Not Reference, for Save Pacakge

	UPackage* OuterPackage = Sprites[0]->GetOutermost();
	const FString SpritePathName = OuterPackage->GetPathName();
	const FString LongPackagePath = FPackageName::GetLongPackagePath(OuterPackage->GetPathName());

	UPaperFlipbookFactory* FlipbookFactory = NewObject<UPaperFlipbookFactory>();
	for (UPaperSprite* Sprite : Sprites)
	{
		if (Sprite != nullptr)
		{
			FPaperFlipbookKeyFrame* KeyFrame = new (FlipbookFactory->KeyFrames) FPaperFlipbookKeyFrame();
			KeyFrame->Sprite = Sprite;
			KeyFrame->FrameRun = 1;
		}
	}

	const FString NewFlipBookDefaultPath = LongPackagePath + TEXT("/") + FlipbookName;
	FString DefaultSuffix;
	FString AssetName;
	FString PackageName;

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(NewFlipBookDefaultPath, DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

	TArray<UObject*> ObjectsToSync;
	UPaperFlipbook* NewFlipBook = nullptr;
	NewFlipBook = Cast<UPaperFlipbook>(AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UPaperFlipbook::StaticClass(), FlipbookFactory));
	if (NewFlipBook)
	{
		ObjectsToSync.Add(NewFlipBook);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
	}

	GWarn->EndSlowTask();

	return NewFlipBook;
}


void UHGGifFactory::CleanUpFactory(const TArray<FBinaryImage>& BinaryImages)
{
	for (const FBinaryImage& BinaryImage : BinaryImages)
	{
		if (BinaryImage.AllocAddress != nullptr)
		{
			FMemory::Free(BinaryImage.AllocAddress);
		}
	}
}
