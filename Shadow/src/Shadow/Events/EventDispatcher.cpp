#include"shpch.hpp"
#include"EventDispatcher.hpp"

#include<mutex>

namespace Shadow
{
	EventDispatcher* s_instance = nullptr;

	EventDispatcher::EventDispatcher()
	{
		m_windowClosedEvents.reserve(5);
		m_windowResizedEvents.reserve(5);
		m_keyEvents.reserve(5);
		m_mouseButtonEvents.reserve(5);
		m_mousePosEvents.reserve(5);
		m_mouseScrolledEvents.reserve(5);
	}

	void EventDispatcher::init()
	{
		s_instance = new EventDispatcher();
	}

	void EventDispatcher::shutdown()
	{
		delete s_instance;
	}

	EventDispatcher& EventDispatcher::get() { return *s_instance; }

	
	void EventDispatcher::addEvent(WindowClosedEvent&& event)
	{
		m_windowClosedEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addEvent(KeyEvent&& event)
	{
		m_keyEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addEvent(MouseButtonEvent&& event)
	{
		m_mouseButtonEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addEvent(MouseMovedEvent&& event)
	{
		m_mousePosEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addEvent(MouseScrolledEvent&& event)
	{
		m_mouseScrolledEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addEvent(WindowResizedEvent&& event)
	{
		m_windowResizedEvents.emplace_back(std::move(event));
	}

	void EventDispatcher::addReciever(const std::function<bool(const WindowClosedEvent&)>& func)
	{
		m_windowClosedEventRecievers.emplace_back(func);
	}

	void EventDispatcher::addReciever(const std::function<bool(const KeyEvent&)>& func)
	{
		m_keyEventRecievers.emplace_back(func);
	}
	void EventDispatcher::addReciever(const std::function<bool(const MouseButtonEvent&)>& func)
	{
		m_mouseButtonEventRecievers.emplace_back(func);
	}

	void EventDispatcher::addReciever(const std::function<bool(const MouseMovedEvent&)>& func)
	{
		m_mousePosEventRecievers.emplace_back(func);
	}

	void EventDispatcher::addReciever(const std::function<bool(const WindowResizedEvent&)>& func)
	{
		m_winResizedEventRecievers.emplace_back(func);
	}

	void EventDispatcher::addReciever(const std::function<bool(const MouseScrolledEvent&)>& func)
	{
		m_mouseScrolledEventRecievers.emplace_back(func);
	}
}