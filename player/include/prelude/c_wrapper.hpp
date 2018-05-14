#pragma once

#include <memory>

template <class T>
using CDeleter = void (*)(T*);

template <class T>
using CResource = std::unique_ptr<T, CDeleter<T>>;

template <class T>
struct CWrapper {
    CWrapper(T* resource_ptr, CDeleter<T> deleter):
        resource(resource_ptr, deleter)
    { }

    auto operator &()   const { return resource.get(); }
    bool init_success() const { return resource.get() != nullptr; }

private:
    using resource_t = CResource<T>;

    resource_t resource;
};
