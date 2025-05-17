 #pragma once

#include"shpch.hpp"
#include"Shadow/Events/EventTypes.hpp"

#include<future>
#include<array>

namespace Shadow
{
	class EventDispatcher
	{
	public:
		static void init();
		static void shutdown();
		static EventDispatcher& get();
		
		void addEvent(WindowClosedEvent&& event);
		void addEvent(WindowResizedEvent&& event);
		void addEvent(KeyEvent&& event);
		void addEvent(MouseButtonEvent&& event);
		void addEvent(MouseMovedEvent&& event);
		void addEvent(MouseScrolledEvent&& event);

		void addReciever(const std::function<bool(const WindowClosedEvent&)>& func);
		void addReciever(const std::function<bool(const WindowResizedEvent&)>& func);
		void addReciever(const std::function<bool(const KeyEvent&)>& func);
		void addReciever(const std::function<bool(const MouseButtonEvent&)>& func);
		void addReciever(const std::function<bool(const MouseMovedEvent&)>& func);
		void addReciever(const std::function<bool(const MouseScrolledEvent&)>& func);

		template<class EventType>
		void dispatch() {}

		template<>
		void dispatch<WindowClosedEvent>()
		{
			if (!m_windowClosedEvents.size())
				return;

			for (auto& reciever : m_windowClosedEventRecievers)
			{
				for (auto& event : m_windowClosedEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_windowClosedEvents.clear();
		}

		template<>
		void dispatch<KeyEvent>()
		{
			if (!m_keyEvents.size())
				return;

			for (auto& reciever : m_keyEventRecievers)
			{
				for (auto& event : m_keyEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_keyEvents.clear();
		}

		template<>
		void dispatch<MouseButtonEvent>()
		{
			if (!m_mouseButtonEvents.size())
				return;

			for (auto& reciever : m_mouseButtonEventRecievers)
			{
				for (auto& event : m_mouseButtonEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_mouseButtonEvents.clear();
		}

		template<>
		void dispatch<MouseMovedEvent>()
		{
			if (!m_mousePosEvents.size())
				return;

			for (auto& reciever : m_mousePosEventRecievers)
			{
				for (auto& event : m_mousePosEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_mousePosEvents.clear();
		}

		template<>
		void dispatch<WindowResizedEvent>()
		{
			if (!m_windowResizedEvents.size())
				return;

			for (auto& reciever : m_winResizedEventRecievers)
			{
				for (auto& event : m_windowResizedEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_windowResizedEvents.clear();
		}

		template<>
		void dispatch<MouseScrolledEvent>()
		{
			if (!m_mouseScrolledEvents.size())
				return;

			for (auto& reciever : m_mouseScrolledEventRecievers)
			{
				for (auto& event : m_mouseScrolledEvents)
					m_handled = reciever(event);

				if (m_handled)
					break;
			}
			m_mouseScrolledEvents.clear();
		}

	private:
		EventDispatcher();
	private:
		bool m_handled = false;

		std::vector<WindowClosedEvent> m_windowClosedEvents;
		std::vector<WindowResizedEvent> m_windowResizedEvents; 
		std::vector<KeyEvent> m_keyEvents;
		std::vector<MouseButtonEvent> m_mouseButtonEvents;
		std::vector<MouseMovedEvent> m_mousePosEvents;
		std::vector<MouseScrolledEvent> m_mouseScrolledEvents;

		std::vector<std::function<bool(const WindowClosedEvent&)>> m_windowClosedEventRecievers;
		std::vector<std::function<bool(const WindowResizedEvent&)>> m_winResizedEventRecievers;
		std::vector<std::function<bool(const KeyEvent&)>> m_keyEventRecievers;
		std::vector<std::function<bool(const MouseButtonEvent&)>> m_mouseButtonEventRecievers;
		std::vector<std::function<bool(const MouseMovedEvent&)>> m_mousePosEventRecievers;
		std::vector<std::function<bool(const MouseScrolledEvent&)>> m_mouseScrolledEventRecievers;
	};
}