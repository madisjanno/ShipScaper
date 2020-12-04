// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentChoiceFunctions.h"
#include "GenericPlatform/GenericPlatformMath.h"

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

TMap<FTileCoordinate, int> runPartPlacementPass(TArray<std::pair<int, FTileCoordinate>>& chosenParts, TSet<FTileCoordinate>& coords, TArray<TSubclassOf<AActor>>& parts, TArray<FPartData>& data, TSet<FTileCoordinate>& unpicked, 
			TSet<FTileCoordinate>& unchecked, TArray<int>& passParts, bool enforceMirror) {
	int N = parts.Num() / 2; // mirrored parts

	TMap<FTileCoordinate, int> partMapping;
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
				partMapping.Add(centre + hex, chosenParts.Num());
				if (unchecked.Contains(centre + hex))
					unchecked.Remove(centre + hex);
			}
			chosenParts.Add({i, centre});

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
					partMapping.Add(mirrorCentre + hex, chosenParts.Num());
					if (unchecked.Contains(mirrorCentre + hex))
						unchecked.Remove(mirrorCentre + hex);
				}

				chosenParts.Add({j, mirrorCentre});
			}

			break;
		}

		if (unchecked.Contains(centre))
			unchecked.Remove(centre);
	}
	return partMapping;
}

std::pair<TArray<std::pair<int, FTileCoordinate>>, TMap<FTileCoordinate, int>> runPartMapping(TArray<FTileCoordinate>& coords, TArray<TSubclassOf<AActor>>& parts, TArray<FPartData>& data) {
	TSet<FTileCoordinate> unpicked(coords);
	TArray<std::pair<int, FTileCoordinate>> chosenParts;

	// Mirrored parts
	int N = parts.Num()/2;

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

	TMap<FTileCoordinate, int> partMapping;

	// Fills symmetric portion of ship
	for (TArray<int>& passParts : passes) {
		TSet<FTileCoordinate> unchecked(symmetric);
		partMapping.Append(runPartPlacementPass(chosenParts, coordsSet, parts, data, symmetric, unchecked, passParts, true));
	}

	
	// Fills rest of ship
	for (TArray<int>& passParts : passes) {
		TSet<FTileCoordinate> unchecked(unpicked);
		partMapping.Append(runPartPlacementPass(chosenParts, coordsSet, parts, data, unpicked, unchecked, passParts, false));
	}
	return {chosenParts, partMapping};
}

struct Chunk
{
	int upper = -100000;
	int lower = 100000;

	TSet<int> chunkParts;

	FTransform transform;

	TSet<int> parents;

	void updateBounds(TArray<std::pair<int, FTileCoordinate>>& placedParts, TArray<std::pair<int, int>>& partSpans) {

		upper = -100000;
		lower = 100000;

		for (int partID: chunkParts) {
			std::pair<int, FTileCoordinate>& part = placedParts[partID];
			int centre = part.second.x;

			if (partSpans[part.first].first+centre > upper)
				upper = partSpans[part.first].first+centre;
			if (partSpans[part.first].second+centre < lower)
				lower = partSpans[part.first].second+centre;
		}
	}
};

void combineChunks(TArray<Chunk>& chunks, int chunkAi, int chunkBi) {

	Chunk& chunkA = chunks[chunkAi];
	Chunk& chunkB = chunks[chunkBi];

	if (chunkA.parents.Contains(chunkBi))
		chunkA.parents.Remove(chunkBi);

	if (chunkB.parents.Contains(chunkAi))
		chunkB.parents.Remove(chunkAi);

	chunkA.chunkParts.Append(chunkB.chunkParts);
	chunkB.chunkParts.Empty();
	chunkA.parents.Append(chunkB.parents);
	chunkB.parents.Empty();

	chunkA.lower = FGenericPlatformMath::Min(chunkA.lower, chunkB.lower);
	chunkA.upper = FGenericPlatformMath::Max(chunkA.upper, chunkB.upper);

	for (Chunk& c: chunks) {
		if (c.parents.Contains(chunkBi)) {
			c.parents.Remove(chunkBi);
			c.parents.Add(chunkAi);
		}
	}
}

TArray<FPartAndTransform> UComponentChoiceFunctions::createShip(TArray<FTileCoordinate> coords, TArray<TSubclassOf<AActor>> parts, TArray<FPartData> data)
{
	// Mirrors parts
	int N = parts.Num();
	for (int i = 0; i < N; i++) {
		TSubclassOf<AActor> actor = parts[i];
		parts.Add(actor);
		data.Add(mirrorPart(data[i]));
	}

	// Precalculates dimensions of parts
	TArray<std::pair<int, int>> partSpans;
	for (FPartData& part: data) {
		int upper = 0;
		int lower = 0;

		for (FTileCoordinate& hex: part.FillSet) {
			if (hex.x > upper)
				upper = hex.x;
			if (hex.x < lower)
				lower = hex.x;
		}

		partSpans.Add({upper, lower});
	}

	// Map chosen hexes to parts
	// (part, centre) and location => *(part,centre)
	auto partsAndMapping = runPartMapping(coords, parts, data);
	TArray<std::pair<int, FTileCoordinate>>& placedParts = partsAndMapping.first;
	TMap<FTileCoordinate, int>& partMapping = partsAndMapping.second;


	TMap<int, int> partToChunk;

	// Create initial chunks
	// Combined with <= => connections
	TArray<Chunk> chunks;
	TSet<int> assignedParts;

	TArray<FTileCoordinate> adjacent;
	adjacent.Add({0, -1});
	adjacent.Add({0, 1});

	// Each unassigned part creates own chunk and adds all adjacent parts
	for (int i = 0; i < placedParts.Num(); i++) {
		if (assignedParts.Contains(i))
			continue;

		chunks.Add(Chunk());
		Chunk& newChunk = chunks.Last();

		TArray<int> partQueue;
		partQueue.Add(i);
		newChunk.chunkParts.Add(i);
		assignedParts.Add(i);
		partToChunk.Add(i, chunks.Num()-1);

		for (int j = 0; j < partQueue.Num(); j++) {
			int partID = partQueue[j];
			FPartData& partData = data[placedParts[partID].first];
			FTileCoordinate& centre = placedParts[partID].second;

			// Iterates through all neighbours of all hexes to find other connected parts
			for (FTileCoordinate& hex: partData.FillSet) {
				for (FTileCoordinate& offset: adjacent) {
					FTileCoordinate potentialNeigbour = centre+hex+offset;

					if (!partMapping.Contains(potentialNeigbour))
						continue;

					int neighbourID = partMapping.FindRef(potentialNeigbour);

					if (!assignedParts.Contains(neighbourID)) {
						partQueue.Add(neighbourID);
						newChunk.chunkParts.Add(neighbourID);
						assignedParts.Add(neighbourID);
						partToChunk.Add(neighbourID, chunks.Num()-1);
					}
				}
			}
		}

		newChunk.updateBounds(placedParts, partSpans);
	}

	// Finds parents
	TArray<FTileCoordinate> upperConnection;
	upperConnection.Add({1, 0});
	upperConnection.Add({1, 1});
	TArray<FTileCoordinate> lowerConnection;
	lowerConnection.Add({-1, -1});
	lowerConnection.Add({-1,  0});

	for (int i = 0; i < chunks.Num(); i++) {
		Chunk& chunk = chunks[i];

		if (chunk.upper >= 0 && chunk.lower <= 0)
			continue;

		for (int partID: chunk.chunkParts) {
			FPartData& partData = data[placedParts[partID].first];
			FTileCoordinate& centre = placedParts[partID].second;

			for (FTileCoordinate& hex: partData.FillSet) {
				if (chunk.upper < 0) {
					for (FTileCoordinate& offset: upperConnection) {
						FTileCoordinate potentialNeigbour = centre+hex+offset;

						if (!partMapping.Contains(potentialNeigbour))
							continue;
						int neighbourID = partMapping.FindRef(potentialNeigbour);

						if (neighbourID == partID || chunk.chunkParts.Contains(neighbourID))
							continue;

						chunk.parents.Add(partToChunk.FindRef(neighbourID));
					}
				} else {
					for (FTileCoordinate& offset: lowerConnection) {
						FTileCoordinate potentialNeigbour = centre+hex+offset;

						if (!partMapping.Contains(potentialNeigbour))
							continue;
						int neighbourID = partMapping.FindRef(potentialNeigbour);

						if (neighbourID == partID || chunk.chunkParts.Contains(neighbourID))
							continue;

						chunk.parents.Add(partToChunk.FindRef(neighbourID));
					}
				}
			}
		}
	}

	// Culls parents until no more changes possible meaning each chunk has 0 or 1 parents
	bool change = true;
	while (change) {
		change = false;
		for (int chunkID = 0; chunkID < chunks.Num(); chunkID++) {
			Chunk& chunk = chunks[chunkID];

			// Combines with parent, if overlapping
			for (int parentID: chunk.parents) {
				Chunk& parentChunk = chunks[parentID];

				if (chunk.lower <= parentChunk.upper && parentChunk.lower <= chunk.upper) {
					if (parentChunk.upper >= chunk.lower) {
						combineChunks(chunks, chunkID, parentID);
						change = true;
						break;
					}
				}
			}

			if (change) 
				continue;
			
			// Combines excess parents
			if (chunk.parents.Num() > 1) {
				for (int parentID: chunk.parents) {
					for (int parentID2: chunk.parents) {
						if (parentID == parentID2)
							continue;

						combineChunks(chunks, parentID, parentID2);
					}
				}
			}
		}
	}





	TArray<FPartAndTransform> shipParts;

	for (Chunk& chunk: chunks) {
		int a = FMath::RandRange(-10, 10);
		chunk.transform.AddToTranslation({0,0,10.0f*a});

		/*
		if (chunk.lower <= 0 && chunk.upper >= 0)
			continue;
		if (chunk.lower > 0)
			chunk.transform.SetLocation({173.2, 0, 0});
		if (chunk.higher < 0)
			chunk.transform.SetLocation({-173.2, 0, 0});
		*/
		//chunk.transform.SetRotation(FQuat(FRotator(10, 0, 0)));
	}
	
	for (Chunk& chunk: chunks) {
		int a = FMath::RandRange(0, 10);

		FTransform totalTransform;
		Chunk* parent = &chunk;

		while (parent) {
			totalTransform *= parent->transform;

			if (parent->parents.Num() > 0) {
				parent = &(chunks[parent->parents.Array()[0]]);
			} else {
				parent = NULL;
			}
		}

		for (int partID: chunk.chunkParts) {
			FPartData& partData = data[placedParts[partID].first];
			FTileCoordinate& centre = placedParts[partID].second;

			FTransform transform(fromHexCoordinate(centre));
			//FTransform transform(fromHexCoordinate(centre-FTileCoordinate(chunk.lower,0)));

			if (placedParts[partID].first >= N)
				transform.MultiplyScale3D({-1,1,1});

			//transform.AddToTranslation({0,0,-20.0f*a});

			shipParts.Add({transform*totalTransform, parts[placedParts[partID].first]});

		}
	}
	
	return shipParts;
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