#include "Knife.h"

void Knife::Initialize(Object3dCommon* object3dCommon, LightManager* lightManager)
{
	GameObject::Initialize(object3dCommon, lightManager);
}

void Knife::Draw(CameraManager* camera)
{
	if (!IsAlive()) return;
	GameObject::Draw3D(camera);
}
