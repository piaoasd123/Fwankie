#include "StaticSpriteComponent.h"

void StaticSpriteComponent::UpdateBounds() {

}

bool StaticSpriteComponent::setStaticSprite(hgeSprite* _staticSprite)
{
	// Do nothing if we are already using the supplied static mesh
	if (_staticSprite == m_pStaticSprite)
	{
		//TODO set up a log
		printf("Sprite already set up for static sprite component.\n");
		return false;
	}

	// Don't allow changing static meshes if "static" and registered
	AActor* Owner = getOwner();
	if (//Mobility == EComponentMobility::Static &&
		isRegistered() && Owner)
	{

		printf("mesh changed failed because it is a static mesh or because it is already registered.\n");
		return false;
	}


	m_pStaticSprite = _staticSprite;

	// TODO: add streaming?

	// Since we have new mesh, we need to update bounds
	UpdateBounds();
	return true;
}

