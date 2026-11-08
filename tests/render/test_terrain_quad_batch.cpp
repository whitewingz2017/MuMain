#include <doctest.h>

#include "Render/Terrain/TerrainQuadBatch.h"

#include <array>
#include <cstring>

namespace
{
std::array<mu::Vertex3D, 4> Quad(float x)
{
    return {{{x, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0xff112233u},
             {x + 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0xff223344u},
             {x + 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0xff334455u},
             {x, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0xff445566u}}};
}
} // namespace

TEST_CASE("ground batching preserves every vertex and groups matching textures [render][terrain][batch]")
{
    Render::TerrainQuadBatch batch;
    const auto first = Quad(10.f);
    const auto other = Quad(20.f);
    const auto last = Quad(30.f);
    batch.Add(1u, first);
    batch.Add(2u, other);
    batch.Add(1u, last);

    int draws = 0;
    const auto lastTexture = batch.Flush(
        [&](std::uint32_t texture, std::span<const mu::Vertex3D> vertices)
        {
            ++draws;
            if (texture == 1u)
            {
                REQUIRE(vertices.size() == 8u);
                CHECK(std::memcmp(vertices.data(), first.data(), sizeof(first)) == 0);
                CHECK(std::memcmp(vertices.data() + 4u, last.data(), sizeof(last)) == 0);
            }
            else
            {
                REQUIRE(texture == 2u);
                REQUIRE(vertices.size() == 4u);
                CHECK(std::memcmp(vertices.data(), other.data(), sizeof(other)) == 0);
            }
        });
    REQUIRE(lastTexture.has_value());
    CHECK(*lastTexture == 1u);
    CHECK(draws == 2);
}

TEST_CASE("ground batches never exceed the renderer's quad index buffer [render][terrain][batch]")
{
    Render::TerrainQuadBatch batch;
    constexpr std::size_t Quads = Render::Topology::MaxQuadsPerDraw + 3u;
    for (std::size_t index = 0; index < Quads; ++index)
        batch.Add(1u, Quad(static_cast<float>(index)));

    std::size_t drawn = 0;
    int draws = 0;
    (void)batch.Flush(
        [&](std::uint32_t texture, std::span<const mu::Vertex3D> vertices)
        {
            CHECK(texture == 1u);
            CHECK(vertices.size() <= Render::Topology::MaxQuadsPerDraw * 4u);
            CHECK(vertices.size() % 4u == 0u);
            for (std::size_t index = 0; index < vertices.size(); index += 4u)
            {
                CHECK(vertices[index].x == static_cast<float>(drawn));
                ++drawn;
            }
            ++draws;
        });
    CHECK(drawn == Quads);
    CHECK(draws == 2);
}

TEST_CASE("flushing ground preserves a cut-out draw between batches [render][terrain][batch]")
{
    Render::TerrainQuadBatch batch;
    std::vector<float> order;
    const auto draw = [&](std::uint32_t, std::span<const mu::Vertex3D> vertices)
    {
        for (std::size_t index = 0; index < vertices.size(); index += 4u)
            order.push_back(vertices[index].x);
    };
    batch.Add(2u, Quad(10.f));
    (void)batch.Flush(draw);
    order.push_back(20.f); // the cut-out tile is drawn immediately
    batch.Add(1u, Quad(30.f));
    (void)batch.Flush(draw);
    CHECK(order == std::vector<float>{10.f, 20.f, 30.f});
}

TEST_CASE("ground batching forgets drawn vertices and empty flushes leave state alone [render][terrain][batch]")
{
    Render::TerrainQuadBatch batch;
    int draws = 0;
    const auto draw = [&](std::uint32_t, std::span<const mu::Vertex3D>) { ++draws; };
    CHECK_FALSE(batch.Flush(draw).has_value());
    batch.Add(1u, Quad(10.f));
    CHECK(batch.Flush(draw).has_value());
    CHECK_FALSE(batch.Flush(draw).has_value());
    batch.Add(2u, Quad(20.f));
    CHECK(batch
              .Flush(
                  [&](std::uint32_t texture, std::span<const mu::Vertex3D> vertices)
                  {
                      ++draws;
                      CHECK(texture == 2u);
                      REQUIRE(vertices.size() == 4u);
                      CHECK(vertices.front().x == 20.f);
                  })
              .has_value());
    CHECK(draws == 2);
}
