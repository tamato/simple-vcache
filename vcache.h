#ifndef VCACHE_H
#define VCACHE_H

#include <deque>
#include <vector>

#include "meshbuffer.h"

namespace sp // Simple and to the Point
{
    class vcache
    {
    public:
        vcache();

        void optimize(const MeshBuffer& buffer);
        void test_result(const MeshBuffer& buffer);

        // returns the new index list
        unsigned int getIndexCount() const;
        const unsigned int * getIndices() const;

    private:

        void _init_scores();
        void _init_verts(const MeshBuffer& buffer);
        void _init_tris(const MeshBuffer& buffer);
        void _score_vertex(int index);
        void _score_triangle(int index);
        int  _find_next_tri();
        void _add_tri_to_LRU(int index);

        float CacheDecayPower;
        float LastTriScore;
        float ValenceBoostScale;
        float ValenceBoostPower;
        int MaxSizeCache;

        struct Vertex
        {
            int maxValence;
            int trisNotAdded;
            int cache_pos;
            float score;
            std::vector<int> reference_list;
        };

        struct Triangle
        {
            bool in_cache;
            float score;
            int referenced_verts[3];
        };

        std::vector<Vertex>       Verts;
        std::vector<Triangle>     Tris;
        std::deque<int>           LRU;
        std::vector<unsigned int> NewTriangleList;
    };
}
#endif // VCACHE_H
