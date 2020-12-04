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

    FTileCoordinate mirrorYaxis() const {
        return FTileCoordinate(-x, y-x);
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
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 mappingPassPriority;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 priority;
};

USTRUCT(BlueprintType)
struct FMappedPart
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FTileCoordinate coordinate;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AActor> part;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool mirrored;

    FMappedPart() {}
    FMappedPart(FTileCoordinate c, TSubclassOf<AActor> p, bool m) : coordinate(c), part(p), mirrored(m) {}
};

USTRUCT(BlueprintType)
struct FPartAndTransform
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FTransform transform;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AActor> part;

    FPartAndTransform() {}
    FPartAndTransform(FTransform t, TSubclassOf<AActor> p) : transform(t), part(p) {}
};

/**
 * 
 */
UCLASS()
class SHIPSCAPER_API UComponentChoiceFunctions : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

        UFUNCTION(BlueprintCallable, Category = "ProcGen")
        static TArray<FPartAndTransform> createShip(TArray<FTileCoordinate> coords, TArray<TSubclassOf<AActor>> parts, TArray<FPartData> data);

        UFUNCTION(BlueprintPure, Category = "ProcGen")
        static FTileCoordinate toHexCoordinate(FVector coords);

        UFUNCTION(BlueprintPure, Category = "ProcGen")
        static FVector fromHexCoordinate(FTileCoordinate coords);
};
