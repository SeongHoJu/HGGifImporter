// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Factories/TextureFactory.h"
#include "HGGifFactory.generated.h"

/**
 * 
 */

USTRUCT()
struct FBinaryImage
{
	GENERATED_USTRUCT_BODY()

	uint8* AllocAddress;
	int64 ImageSize;
};

typedef void*(*_CallbackRequestBitmapAlloc)(int32 FrameIndex, int64 ImageSize);
typedef int32(*_DecodeFunction)(void* GifBinaires, int32 BinarySize, _CallbackRequestBitmapAlloc CallbackAlloc); // Declare the DLL function.


UCLASS()
class HGGIFIMPORTER_API UHGGifFactory : public UTextureFactory
{
	GENERATED_UCLASS_BODY()
	
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
	
	void*						GeneratedDllHandle;
	_DecodeFunction				GifDecodeFunction;
	_CallbackRequestBitmapAlloc CallbackRequestAlloc;

	static TArray<FBinaryImage> CachedBinaryImages;

	/*
		Called by C# dll ( Register this function to C# dll(GifConverrter.dll) )
	*/
	static void* CallbackRequestHeapAlloc(int32 FrameIndex, int64 BitmapSize);

	/*
		Load Dll Module
	*/
	virtual bool GenerateConverterDll();

	/*
		Decode Gif to frame Image using c# dll
	*/
	virtual bool DecodeGifFile(const TArray<uint8>& GifBytes, TArray<FBinaryImage>& GifFrames);

	/*
		Create Texture from Decoded Gif Frame Image
	*/
	virtual bool GenerateTextures(const TArray<FBinaryImage>& BinaryImages, /*out*/ TArray<UTexture2D*>& OutTextures, UObject* InParent, FName Name, const TCHAR* Type, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
	
	/*
		Create Single Sprite
	*/
	virtual class UPaperSprite* CreateNewSprite(UTexture2D* InitialTexture, UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);

	/*
		Generate TextureList from SpriteList
	*/
	virtual bool GenerateSprites(const TArray<UTexture2D*>& Textures, /*out*/TArray<UPaperSprite*>& OutSprites, UObject* InParent, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
	
	/*
		Create Flipbook from Sprite List
	*/
	virtual class UPaperFlipbook* CreateFlipBook(const TArray<UPaperSprite*>& AllSprites, FName BaseName);

	/*
		clean up allocated Image and Cached Static Variable
	*/
	virtual void CleanUpFactory(const TArray<FBinaryImage>& BinaryImages);
};
