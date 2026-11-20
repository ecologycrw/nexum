#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "Nexum/Core/Handle.hpp"
#include "Nexum/Core/HandlePool.hpp"

using nexum::core::Handle;
using nexum::core::HandlePool;

namespace {

struct Texture {
    std::string name;
};

struct Voice {
    int channel = 0;
};

[[nodiscard]] Handle<Texture> AddTexture(HandlePool<Texture>& pool, const char* name) {
    auto added = pool.Add(Texture{name});
    REQUIRE(added.has_value());
    return *added;
}

} // namespace

TEST_CASE("[0000-CA-04] a handle finds its object until the object is removed") {
    HandlePool<Texture> pool{4};
    const Handle<Texture> grass = AddTexture(pool, "grass");
    const Handle<Texture> stone = AddTexture(pool, "stone");
    CHECK(pool.Size() == 2);

    REQUIRE(pool.Find(grass) != nullptr);
    CHECK(pool.Find(grass)->name == "grass");
    CHECK(pool.Find(stone)->name == "stone");

    CHECK(pool.Remove(grass));
    CHECK(pool.Find(grass) == nullptr);
    CHECK_FALSE(pool.Contains(grass));
    CHECK_FALSE(pool.Remove(grass));
    CHECK(pool.Find(stone)->name == "stone");
    CHECK(pool.Size() == 1);
}

TEST_CASE("[0000-CA-04] the handle of a removed object is refused even after its slot is reused") {
    HandlePool<Texture> pool{4};
    const Handle<Texture> old = AddTexture(pool, "old");
    REQUIRE(pool.Remove(old));

    const Handle<Texture> reused = AddTexture(pool, "new");
    REQUIRE(reused.index == old.index);
    CHECK(reused.generation != old.generation);

    CHECK(pool.Find(old) == nullptr);
    CHECK_FALSE(pool.Remove(old));
    REQUIRE(pool.Find(reused) != nullptr);
    CHECK(pool.Find(reused)->name == "new");
}

TEST_CASE("[0000-CA-04] a full pool returns an error instead of growing") {
    HandlePool<Texture> pool{2};
    const Handle<Texture> first = AddTexture(pool, "first");
    static_cast<void>(AddTexture(pool, "second"));

    const auto refused = pool.Add(Texture{"third"});
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the pool is full (2 objects)");
    CHECK(pool.Size() == 2);
    CHECK(pool.Capacity() == 2);

    // Removing an object makes room again.
    REQUIRE(pool.Remove(first));
    CHECK(pool.Add(Texture{"third"}).has_value());
}

TEST_CASE("a pool with no room is always full") {
    HandlePool<Texture> pool{0};
    CHECK_FALSE(pool.Add(Texture{"any"}).has_value());
}

TEST_CASE("a handle that refers to nothing is refused") {
    HandlePool<Texture> pool{2};
    static_cast<void>(AddTexture(pool, "grass"));

    // The default handle, and a handle past the end of the pool.
    for (const Handle<Texture> handle : {Handle<Texture>{}, Handle<Texture>{.index = 7, .generation = 1}}) {
        CAPTURE(handle.index);
        CHECK(pool.Find(handle) == nullptr);
        CHECK_FALSE(pool.Contains(handle));
        CHECK_FALSE(pool.Remove(handle));
    }
    CHECK(pool.Size() == 1);
}

TEST_CASE("the objects stay in place while others are added and removed") {
    HandlePool<Texture> pool{8};
    const Handle<Texture> kept = AddTexture(pool, "kept");
    const Texture* const address = pool.Find(kept);

    std::vector<Handle<Texture>> others;
    for (int round = 0; round < 3; ++round) {
        for (int added = 0; added < 7; ++added) {
            others.push_back(AddTexture(pool, "other"));
        }
        for (const Handle<Texture> other : others) {
            REQUIRE(pool.Remove(other));
        }
        others.clear();
    }

    CHECK(pool.Find(kept) == address);
    CHECK(pool.Find(kept)->name == "kept");
}

TEST_CASE("a const pool finds its objects too") {
    HandlePool<Texture> pool{1};
    const Handle<Texture> grass = AddTexture(pool, "grass");
    const HandlePool<Texture>& view = pool;
    REQUIRE(view.Find(grass) != nullptr);
    CHECK(view.Find(grass)->name == "grass");
}

TEST_CASE("a handle of one kind of object never stands for another kind") {
    static_assert(!std::is_convertible_v<Handle<Texture>, Handle<Voice>>);
    static_assert(!std::is_constructible_v<Handle<Voice>, Handle<Texture>>);
    static_assert(sizeof(Handle<Texture>) == 2 * sizeof(std::uint32_t));
    CHECK(Handle<Texture>{} == Handle<Texture>{});
}
