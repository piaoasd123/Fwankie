#include "Actor.h"
#include "TileMap.h"
#include "ScenePrivate.h"
#include "SpriteComponent.h"

void SpriteComponent::UpdateBounds() {

}

bool SpriteComponent::setStaticSprite(hgeSprite* _staticSprite)
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
	if (Owner != NULL && isRegistered())
	{
		printf("mesh changed failed because it is a static mesh or because it is already registered.\n");
		return false;
	}


	m_pStaticSprite = _staticSprite;

	// TODO: add streaming?

	// Since we have new mesh, we need to update bounds
	//UpdateBounds();
	return true;
}

SpriteComponent::SpriteComponent()
{

}

SpriteComponent::~SpriteComponent()
{

}

void SpriteComponent::Render(float x, float y)
{
	m_pStaticSprite->Render(x, y);
}

StaticSpriteSceneProxy* SpriteComponent::Create_SceneProxy()
{
	return NULL;
}

void SpriteComponent::Tick(float _DeltaTime)
{
	m_pStaticSprite->Update(_DeltaTime);
}

void SpriteComponent::initWithSpriteName(const TCHAR* _name)
{

}

void SpriteComponent::OnRegister()
{
	assert(m_pOwner && m_pOwner->getCurrMap());
	m_pOwner->getCurrMap()->getScene()->AddStaticSprite(this);
}
