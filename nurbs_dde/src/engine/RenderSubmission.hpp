#pragma once
// engine/RenderSubmission.hpp
// Small renderer-neutral helpers over EngineAPI arena allocation/submission.

#include "engine/EngineAPI.hpp"

#include <vector>

namespace ndde {

template <class WriteVertex>
inline void submit_generated_vertices(EngineAPI& api,
                                      RenderTarget target,
                                      u32 vertex_count,
                                      Topology topology,
                                      DrawMode mode,
                                      Vec4 color,
                                      Mat4 mvp,
                                      WriteVertex&& write_vertex)
{
    if (vertex_count == 0) return;
    auto slice = api.acquire(vertex_count);
    for (u32 i = 0; i < vertex_count; ++i)
        write_vertex(slice.vertices()[i], i);
    api.submit_to(target, slice, topology, mode, color, mvp);
}

inline void submit_vertices(EngineAPI& api,
                            RenderTarget target,
                            const std::vector<Vertex>& vertices,
                            u32 vertex_count,
                            Topology topology,
                            DrawMode mode,
                            Vec4 color,
                            Mat4 mvp)
{
    submit_generated_vertices(api, target, vertex_count, topology, mode, color, mvp,
        [&vertices](Vertex& out, u32 i) {
            out = vertices[i];
        });
}

template <class TransformVertex>
inline void submit_transformed_vertices(EngineAPI& api,
                                        RenderTarget target,
                                        const std::vector<Vertex>& vertices,
                                        u32 vertex_count,
                                        Topology topology,
                                        DrawMode mode,
                                        Vec4 color,
                                        Mat4 mvp,
                                        TransformVertex&& transform_vertex)
{
    submit_generated_vertices(api, target, vertex_count, topology, mode, color, mvp,
        [&vertices, &transform_vertex](Vertex& out, u32 i) {
            out = vertices[i];
            transform_vertex(out, i);
        });
}

} // namespace ndde
