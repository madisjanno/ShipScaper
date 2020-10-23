// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentChoiceFunctions.h"

TMap<FTileCoordinate, int32> UComponentChoiceFunctions::createComponentMapping(TArray<FTileCoordinate> coords) {
	TSet<FTileCoordinate> unpicked(coords);
	int32 counter = 0;
	TMap<FTileCoordinate, int32> mapping;


	TArray<TSet<FTileCoordinate>> possibleComponents;
	
	TSet<FTileCoordinate> comp1;
	comp1.Add(FTileCoordinate(0, 0));
	comp1.Add(FTileCoordinate(1, 0));
	comp1.Add(FTileCoordinate(-1, 0));
	possibleComponents.Push(comp1);

	TSet<FTileCoordinate> comp2;
	comp2.Add(FTileCoordinate(0, 0));
	comp2.Add(FTileCoordinate(0, -1));
	comp2.Add(FTileCoordinate(0, 1));
	possibleComponents.Push(comp2);
	
	TSet<FTileCoordinate> comp3;
	comp3.Add(FTileCoordinate(0, 0));
	possibleComponents.Push(comp3);
	
	while (unpicked.Num() > 0) {
		
		FTileCoordinate centre = unpicked.Array()[FMath::RandRange(0, unpicked.Num()-1)];

		for (auto& comp : possibleComponents) {
			bool fit = true;
			for (auto& hex : comp) {
				if (!unpicked.Contains(centre + hex)) {
					fit = false;
					break;
				}
			}
			if (fit) {
				for (auto& hex : comp) {
					unpicked.Remove(centre + hex);
					mapping.Add(centre + hex, counter);
				}
				counter++;
				break;
			}
		}
	}
	
	return mapping;
}

TMap<FTileCoordinate, TSubclassOf<AActor>> UComponentChoiceFunctions::createPartMapping(TArray<FTileCoordinate> coords, TArray<TSubclassOf<AActor>> parts, TArray<FPartData> data) {
	TSet<FTileCoordinate> unpicked(coords);
	TMap<FTileCoordinate, TSubclassOf<AActor>> chosenParts;

	while (unpicked.Num() > 0) {

		FTileCoordinate centre = unpicked.Array()[FMath::RandRange(0, unpicked.Num() - 1)];

		for (int i = 0; i < parts.Num(); i++) {
			bool fit = true;

			// Will fill this area
			for (auto& hex : data[i].FillSet) {
				if (!unpicked.Contains(centre + hex)) {
					fit = false;
					break;
				}
			}
			if (!fit)
				continue;

			// Should be tiles in this area
			for (auto& hex : data[i].RequiredSet) {
				if (!coords.Contains(centre + hex)) {
					fit = false;
					break;
				}
			}
			if (!fit)
				continue;

			// Shouldn't be tiles here
			for (auto& hex : data[i].BlockedSet) {
				if (coords.Contains(centre + hex)) {
					fit = false;
					break;
				}
			}
			if (!fit)
				continue;

			// Fill tiles and add part
			for (auto& hex : data[i].FillSet) {
				unpicked.Remove(centre + hex);
			}
			chosenParts.Add(centre, parts[i]);
			break;
		}
	}

	return chosenParts;
}

FTileCoordinate UComponentChoiceFunctions::toHexCoordinate(FVector coords) {
	float Xmult = 173.2;
	float Ymult = 200;

	int x = round(coords.X / Xmult);
	int y = round( (coords.Y+x*Ymult/2.0) / Ymult);
	return FTileCoordinate(x, y);
}

FVector UComponentChoiceFunctions::fromHexCoordinate(FTileCoordinate coords) {
	float Xmult = 173.2;
	float Ymult = 200;

	float x = coords.x * Xmult;
	float y = coords.y * Ymult - coords.x * Ymult / 2.0;

	return FVector(x, y, 0);
}