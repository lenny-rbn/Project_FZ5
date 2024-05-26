#include "S_SlicedMesh.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"


AS_SlicedMesh::AS_SlicedMesh()
{
	// Create a procedural mesh.
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	SetupMesh(ProceduralMesh, false, false, false);
	ProceduralMesh->bUseComplexAsSimpleCollision = false;
	RootComponent = ProceduralMesh;

	// Create a static mesh.
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetupMesh(StaticMesh, true, false, false);
	StaticMesh->SetupAttachment(ProceduralMesh);

	// Use the default cube static mesh.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCube(TEXT("/Engine/BasicShapes/Cube"));
	StaticMesh->SetStaticMesh(DefaultCube.Object);
}

void AS_SlicedMesh::BeginPlay()
{
	Super::BeginPlay();

	// Copy the static mesh data to the procedural mesh.
	ProceduralMesh->ClearAllMeshSections();
	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(StaticMesh, 0, ProceduralMesh, true);

	// Hide the static mesh and make the procedural mesh visible, tangible but not simulated.
	SetupMesh(StaticMesh, false, false, false);
	SetupMesh(ProceduralMesh, true, true, false);
}

void AS_SlicedMesh::Slice(UProceduralMeshComponent* ProcMesh, FVector PlanePosition, FVector PlaneNormal)
{
	if (ProcMesh->GetOwner() != this) return;

	const bool bIsGrounded = (ProcMesh == ProceduralMesh/* && !ProceduralMesh->IsSimulatingPhysics()*/);

	// Slice the procedural mesh in half along the given plane.
	UProceduralMeshComponent* NewProcMesh = nullptr;
	UKismetProceduralMeshLibrary::SliceProceduralMesh(ProcMesh, PlanePosition, PlaneNormal, true, NewProcMesh,
		EProcMeshSliceCapOption::CreateNewSectionForCap,
		ProceduralMesh->GetMaterial(0));
	if (!NewProcMesh || !NewProcMesh->IsValidLowLevel()) return;

	// Find the lower and higher parts of the sliced procedural mesh.
	const FVector Pos1 = ProcMesh->GetComponentLocation();
	const FVector Pos2 = NewProcMesh->GetComponentLocation();
	UProceduralMeshComponent* LowerProceduralMesh = (Pos1.Z <= Pos2.Z ? NewProcMesh : ProcMesh);
	UProceduralMeshComponent* UpperProceduralMesh = (LowerProceduralMesh == ProcMesh ? NewProcMesh : ProcMesh);

	// Make sure that a grounded procedural mesh slice stays grounded and non simulated.
	if (bIsGrounded)
	{
		ProceduralMesh = LowerProceduralMesh;
	}
	else
	{
		SetupMesh(LowerProceduralMesh, true, true, true);
		LowerProceduralMesh->AddImpulse(/*{ 0, 0, 1000 }*/(-PlaneNormal / PlaneNormal.Size()) * 1000, NAME_None, true);
	}

	// Enable simulation for the upper procedural mesh slice and move it up.
	SetupMesh(UpperProceduralMesh, true, true, true);
	UpperProceduralMesh->AddImpulse(/*{ 0, 0, 1000 }*/(PlaneNormal / PlaneNormal.Size()) * 1000, NAME_None, true);
}

void AS_SlicedMesh::SetupMesh(UMeshComponent* Mesh, bool bVisible, bool bCollision, bool bSimulated)
{
	Mesh->SetVisibility(bVisible);
	Mesh->CastShadow = bVisible;
	Mesh->SetSimulatePhysics(bSimulated);
	Mesh->SetNotifyRigidBodyCollision(bSimulated && bCollision);
	Mesh->SetGenerateOverlapEvents(bCollision);
	Mesh->SetCollisionResponseToAllChannels(bCollision ? ECR_Block : ECR_Ignore);
	Mesh->CanCharacterStepUpOn = bCollision ? ECB_Yes : ECB_No;
}