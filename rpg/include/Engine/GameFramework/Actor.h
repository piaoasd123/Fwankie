#pragma once
#include "Object.h"
#include <vector>
#include <map>

//Component
class IActorComponent;

//Level
class Map;

//ComponentTickScript
class FActorComponentTickFunc;


class AActor : public Object
{
public:
    AActor();
	virtual ~AActor() {}

	//Tick Functions
	//default tick
	virtual void Tick();

	//tick with given script(functor)
	virtual void Tick(std::map<IActorComponent*, FActorComponentTickFunc*>& _tickScript);

	//tick script(functor)
	virtual void registerTickScript(std::map<IActorComponent*, FActorComponentTickFunc*>& _tickScript);

	virtual void unregisterTickScript(IActorComponent* _unregisterTarget);

	//Life Spans
	virtual float getBirthTime();

	virtual void setBirthTime(float _birthTime);

	virtual float getLifeSpan();

	virtual void setLifeSpan(float _lifeSpan);

	//register with level
	virtual void registerWithMap();

	virtual void unregisterWithMap();

	virtual void registerComponentsWithMap(IActorComponent* _component);

	virtual void unregisterComponentWithMap(IActorComponent* _component);

	//get specific component
	template<class T>
	void getComponent(std::vector<T*>& _outComponent);

	template<class T>
	void getComponent(T* _component);

	virtual void getRootComponent();

	virtual void getAllComponent(std::vector<IActorComponent*>& _outComponent);

	//modify component
	virtual void addComponent(IActorComponent* _component);

	virtual void removeComponent(IActorComponent* _component);

	virtual void clearComponent();

	AActor(IActorComponent* _rootComponent, float _lifeSpan, Map* _map) :
		m_RootComponent(_rootComponent), m_lifeSpan(_lifeSpan), m_map(_map) {}

private:
    //actor flags
    bool m_bEnableCollision = false;
	bool m_bEnableRender = false;
	bool m_bBlockUserInput = false;
	bool m_bDestructible = false;

	//component
	IActorComponent* m_RootComponent = nullptr;
	std::vector<IActorComponent*> m_OwnedComponents;

	//lifespan, if destructible
	//if lifespan is set to 0, this Actor will be destroyed when certain circumstance happens
	float m_lifeSpan = 0;
	float m_birthTime = 0;

	//default tick script(functor)
	std::map<IActorComponent*, FActorComponentTickFunc*> m_tickScript;

	//Environment: level
	Map* m_map = nullptr;
};