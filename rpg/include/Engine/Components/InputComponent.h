#include "hge.h"
#include "VarArgDelegate.h"
#include "ActorComponent.h"
#include <map>
#include <queue>

class Character;

//typedef void(AActor::*ActorActionFunc)();

class InputComponent : public IActorComponent {
public:
	InputComponent();

	InputComponent(Character* _owner);

	void addKeyBinding(std::tstring _command, Callback<void()> _delegate);

	void addCommand(std::tstring _command) {
		m_commandQueue.push(_command);
	}

	void executeAction();

private:
	std::map<std::tstring, Callback<void()> > m_movementKeyMapping;
	std::queue<std::tstring> m_commandQueue;
};