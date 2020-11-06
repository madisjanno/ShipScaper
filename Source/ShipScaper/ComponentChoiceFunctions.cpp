// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentChoiceFunctions.h"

FPartData mirrorPart(FPartData part) {
	FPartData mpart = part;

	mpart.BlockedSet.Empty();
	for (FTileCoordinate& c : part.BlockedSet) {
		mpart.BlockedSet.Add(c.mirrorYaxis());
	}

	mpart.FillSet.Empty();
	for (FTileCoordinate& c : part.FillSet) {
		mpart.FillSet.Add(c.mirrorYaxis());
	}

	mpart.RequiredSet.Empty();
	for (FTileCoordinate& c : part.RequiredSet) {
		mpart.RequiredSet.Add(c.mirrorYaxis());
	}

	return mpart;
}

bool checkPartFit(FPartData part, FTileCoordinate centre, TSet<FTileCoordinate>& coords, TSet<FTileCoordinate>& unpicked) {
	for (auto& hex : part.FillSet) {
		if (!unpicked.Contains(centre + hex))
			return false;
	}

	for (auto& hex : part.RequiredSet) {
		if (!coords.Contains(centre + hex))
			return false;
	}

	for (auto& hex : part.BlockedSet) {
		if (coords.Contains(centre + hex))
			return false;
	}

	return true;
}

void runPass(TArray<FMappedPart>& chosenParts, TSet<FTileCoordinate>& coords, TArray<TSubclassOf<AActor>>& parts, TArray<FPartData>& data, TSet<FTileCoordinate>& unpicked, 
			TSet<FTileCoordinate>& unchecked, TArray<int>& passParts, bool enforceMirror) {
	int N = parts.Num() / 2; // mirrored parts
	// Randomly checks all given locations
	while (unchecked.Num() > 0) {
		FTileCoordinate centre = unchecked.Array()[FMath::RandRange(0, unchecked.Num() - 1)];

		// Tries to fit part into location
		for (int i : passParts) {
			if (!checkPartFit(data[i], centre, coords, unpicked))
				continue;

			// Mirror check
			if (enforceMirror) {
				int j;
				if (i >= N)
					j = i - N;
				else
					j = i + N;

				if (!checkPartFit(data[j], centre.mirrorYaxis(), coords, unpicked))
					continue;
			}

			bool fit = true;
			if (enforceMirror) {
				for (auto& hex : data[i].FillSet) {
					// Check symmetry on centerline
					if (centre.x == 0) {
						if (!data[i].FillSet.Contains(hex.mirrorYaxis())) {
							fit = false;
							break;
						}
					}
					else { // Check no flow over centerline
						if (centre.x > 0 != (centre + hex).x > 0) {
							fit = false;
							break;
						}
						if (centre.x < 0 != (centre + hex).x < 0) {
							fit = false;
							break;
						}
					}
				}
			}
			if (!fit)
				continue;

			// Fill tiles and add part
			for (auto& hex : data[i].FillSet) {
				unpicked.Remove(centre + hex);
				if (unchecked.Contains(centre + hex))
					unchecked.Remove(centre + hex);
			}
			chosenParts.Add(FMappedPart(centre, parts[i], i >= N));

			// Fill symmetric tiles and add part
			if (enforceMirror && centre.x != 0) {
				FTileCoordinate mirrorCentre = centre.mirrorYaxis();

				int j;
				if (i >= N)
					j = i - N;
				else
					j = i + N;

				for (auto& hex : data[j].FillSet) {
					unpicked.Remove(mirrorCentre + hex);
					if (unchecked.Contains(mirrorCentre + hex))
						unchecked.Remove(mirrorCentre + hex);
				}

				chosenParts.Add(FMappedPart(mirrorCentre, parts[j], j >= N));
			}

			break;
		}

		if (unchecked.Contains(centre))
			unchecked.Remove(centre);
	}
}

TArray<FMappedPart> UComponentChoiceFunctions::createPartMapping(TArray<FTileCoordinate> coords, TArray<TSubclassOf<AActor>> parts, TArray<FPartData> data) {
	TSet<FTileCoordinate> unpicked(coords);
	TArray<FMappedPart> chosenParts;

	// Mirrors parts
	int N = parts.Num();
	for (int i = 0; i < N; i++) {
		TSubclassOf<AActor> actor = parts[i];
		parts.Add(actor);
		data.Add(mirrorPart(data[i]));
	}

	// Creates array of indices which is easier to sort
	TArray<int> indices;
	for (int i = 0; i < parts.Num(); i++) {
		indices.Add(i);
	}

	// Sorts according to priorities
	indices.Sort([&](const int& lhs, const int& rhs) {
		if (data[lhs].mappingPassPriority == data[rhs].mappingPassPriority)
			return data[lhs].priority > data[rhs].priority;
		else
			return data[lhs].mappingPassPriority > data[rhs].mappingPassPriority;
	});

	// Creates list of passes to do
	TArray<TArray<int>> passes;
	int lastPass = -1;
	for (int i: indices) {
		if (lastPass != data[i].mappingPassPriority)
			passes.Add(TArray<int>());
		passes.Last().Add(i);
	}
	
	TSet<FTileCoordinate> coordsSet(coords);

	// Chooses only symmetric set of coords
	TSet<FTileCoordinate> symmetric;
	for (auto& c : coordsSet) {
		if (coordsSet.Contains(c.mirrorYaxis())) {
			symmetric.Add(c);
			unpicked.Remove(c);
		}
	}

	// Fills symmetric portion of ship
	for (TArray<int>& passParts : passes) {
		TSet<FTileCoordinate> unchecked(symmetric);
		runPass(chosenParts, coordsSet, parts, data, symmetric, unchecked, passParts, true);
	}

	
	// Fills rest of ship
	for (TArray<int>& passParts : passes) {
		TSet<FTileCoordinate> unchecked(unpicked);
		runPass(chosenParts, coordsSet, parts, data, unpicked, unchecked, passParts, false);
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