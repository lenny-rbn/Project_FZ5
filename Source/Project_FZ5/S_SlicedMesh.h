#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "S_SlicedMesh.generated.h"


UCLASS()
class PROJECT_FZ5_API AS_SlicedMesh : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UProceduralMeshComponent* ProceduralMesh = nullptr;
	
public:	
	AS_SlicedMesh();

protected:
	virtual void BeginPlay() override;

public:	
	void Slice(UProceduralMeshComponent* ProcMesh, FVector PlanePosition, FVector PlaneNormal);
	void SetupMesh(UMeshComponent* Mesh, bool bVisible, bool bCollision, bool bSimulated);
};