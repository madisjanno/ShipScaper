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
		/*
		FTileCoordinate centre = unpicked.Array()[FMath::RandRange(0, unpicked.Num()-1)];
		
		mapping.Add(centre, counter);
		
		int c = 1;
		while (unpicked.Contains(centre + FTileCoordinate(0, c))) {
			mapping.Add(centre + FTileCoordinate(0, c), counter);
			unpicked.Remove(centre + FTileCoordinate(0, c));
			c++;
		}
		c = -1;
		while (unpicked.Contains(centre + FTileCoordinate(0, c))) {
			mapping.Add(centre + FTileCoordinate(0, c), counter);
			unpicked.Remove(centre + FTileCoordinate(0, c));
			c--;
		}
		
		unpicked.Remove(centre);
		counter++;
		*/
	}
	
	return mapping;
}