// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ComponentChoiceFunctions.generated.h"

USTRUCT(BlueprintType)
struct FTileCoordinate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 x;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 y;

    FTileCoordinate() : x(0), y(0) {}
    FTileCoordinate(int32 x, int32 y) : x(x), y(y) {}

    bool operator== (const FTileCoordinate& Other) const
    {
        return ((this->x == Other.x) && (this->y == Other.y));
    }

    FTileCoordinate operator+ (const FTileCoordinate& Other) const {
        return FTileCoordinate(x + Other.x, y + Other.y);
    }

    FTileCoordinate operator- (const FTileCoordinate& Other) const {
        return FTileCoordinate(x - Other.x, y - Other.y);
    }

    friend uint32 GetTypeHash(const FTileCoordinate& Other)
    {
        return GetTypeHash(Other.x)+GetTypeHash(Other.y);
    }
};

USTRUCT(BlueprintType)
struct FPartData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSet<FTileCoordinate> RequiredSet;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSet<FTileCoordinate> BlockedSet;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSet<FTileCoordinate> FillSet;
};

/**
 * 
 */
UCLASS()
class SHIPSCAPER_API UComponentChoiceFunctions : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

        UFUNCTION(BlueprintPure, Category = "ProcGen")
        static TMap<FTileCoordinate, int32> createComponentMapping(TArray<FTileCoordinate> coords);

        UFUNCTION(BlueprintCallable, Category = "ProcGen")
        static TMap<FTileCoordinate, TSubclassOf<AActor>> createPartMapping(TArray<FTileCoordinate> coords, TArray<TSubclassOf<AActor>> parts, TArray<FPartData> data);

        UFUNCTION(BlueprintPure, Category = "ProcGen")
        static FTileCoordinate toHexCoordinate(FVector coords);

        UFUNCTION(BlueprintPure, Category = "ProcGen")
        static FVector fromHexCoordinate(FTileCoordinate coords);
};
