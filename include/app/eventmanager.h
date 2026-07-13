#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <type_traits>
#include <unordered_map>

using Event = std::function<void()>;

class EventRegistry {
public:
  static EventRegistry& Get () {
    static EventRegistry instance;
    return instance;
  }

  void RegisterEvent(Event function);
  void TriggerEvents();

private:
  std::vector<Event> mRegistry;

public:
  EventRegistry(const EventRegistry &) = delete;
  EventRegistry &operator=(const EventRegistry &) = delete;
  EventRegistry(EventRegistry &&) = delete;
  EventRegistry &operator=(EventRegistry &&) = delete;

private:
  EventRegistry() = default;
	~EventRegistry() = default;
};

template<typename Signature>
class FunctionRegistry;

template<typename Ret, typename... Args>
class FunctionRegistry<Ret (Args...)> {
public:
  using FunctionType = std::function<Ret(Args...)>;
  void Register (const std::string& label, FunctionType fn) {
    if (Contains (label)) {
      std::fprintf(stderr, "[FunctionRegistry] overwriting \"%s\"\n", label.c_str());
    }
    mFunctions[label] = std::move(fn);
  }

  void Unregister (const std::string& label) {
    mFunctions.erase(label);
  }

  bool Contains (const std::string& label) {
    return mFunctions.find(label) != mFunctions.end();
  }

  Ret Call (const std::string& label, Args... args) const {
    auto it = mFunctions.find(label);
    if (it == mFunctions.end ()) {
      std::fprintf(stderr, "[FunctionRegistry] no function registered under \"%s\"\n", label.c_str());
      if constexpr (!std::is_void_v<Ret>)
        return Ret{};
      else
        return;
    }
    return it->second(args...);
  }

  const std::unordered_map<std::string, FunctionType>& GetAll () const {
    return mFunctions;
  }

private:
  std::unordered_map<std::string, FunctionType> mFunctions;
};

