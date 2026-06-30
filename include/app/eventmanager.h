#pragma once

#include <string>
#include <vector>
#include <functional>

class EventRegistry {
public:
  static EventRegistry& Get () {
    static EventRegistry instance;
    return instance;
  }

  void RegisterEvent(std::function<void()> function);
  void TriggerEvents();

private:
  std::vector<std::function<void()>> mRegistry;

public:
  EventRegistry(const EventRegistry &) = delete;
  EventRegistry &operator=(const EventRegistry &) = delete;
  EventRegistry(EventRegistry &&) = delete;
  EventRegistry &operator=(EventRegistry &&) = delete;

private:
  EventRegistry() = default;
	~EventRegistry() = default;
};
