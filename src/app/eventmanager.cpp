#include <app/eventmanager.h>
#include <cassert>

void EventRegistry::RegisterEvent(std::function<void ()> function) {
  mRegistry.push_back(function);
}

void EventRegistry::TriggerEvents () {
  for (auto &func : mRegistry)
    func();
  mRegistry.clear();
}
