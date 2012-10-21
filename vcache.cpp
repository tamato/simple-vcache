#include <iostream>
#include <assert.h>

#include "vcache.h"

using namespace std;
using namespace sp;

vcache::vcache()
    : CacheDecayPower(1.5f)
    , LastTriScore(0.75f)
    , ValenceBoostScale(2.0f)
    , ValenceBoostPower(0.5f)
    , MaxSizeCache(35)
{
}

void vcache::optimize(const MeshBuffer& buffer)
{
    // start init'ing
    cout << "[ ] Initializing verts" << endl;
    _init_verts(buffer);

    cout << "[ ] Initializing triangles" << endl;
    _init_tris(buffer);

    // get the scores going
    cout << "[ ] Initializing scores" << endl;
    _init_scores();

    cout << "[ ] Verts: " << buffer.getVertCnt() << " Triangles: " << buffer.getIdxCnt() / 3 << endl;

    cout << "[ ] Optimizing..." << endl;
    while (true) {
        int next_tri = _find_next_tri();
        if (next_tri < 0) break;
        _add_tri_to_LRU(next_tri);
    }
    cout << "[!] Optimizing finised" << endl;
}

void vcache::test_result(const MeshBuffer& buffer)
{
    cout << "[ ] Comparing results between the optimized mesh and non optimized mesh.\n"
         << "[-] This will report the ACMR's for the before and after, the lower the better"
         << endl;

    int non_opt_misses = 0;
    int opt_misses = 0;
    bool found = false;
    std::deque<int> fifo;

    // first the non optimized mesh
    const unsigned int *indices = buffer.getIndices();
    for (int i=0; i<buffer.getIdxCnt(); ++i) {
        found = false;

        int vert_idx = indices[i];
        for (int j=0; j<fifo.size(); ++j) {
            if (vert_idx == fifo[j]) {
                found = true;
                break;
            }
        }

        // if not found keep track, and add it to the deque
        if (found == false) {
            non_opt_misses++;
            fifo.push_back(vert_idx);

            // be sure to keep the size contained
            int difference = (int)fifo.size() - (MaxSizeCache-3);
            for (int i=0; i<difference; ++i)
                fifo.pop_front();
        }
    }

    // and now the optimized one
    for (int i=0; i<(int)NewTriangleList.size(); ++i) {
        found = false;

        int vert_idx = NewTriangleList[i];
        for (int j=0; j<fifo.size(); ++j) {
            if (vert_idx == fifo[j]) {
                found = true;
                break;
            }
        }

        // if not found keep track, and add it to the deque
        if (found == false) {
            opt_misses++;
            fifo.push_back(vert_idx);

            // be sure to keep the size contained
            int difference = (int)fifo.size() - (MaxSizeCache-3);
            for (int i=0; i<difference; ++i)
                fifo.pop_front();
        }
    }

    const float triangle_count = (float)Tris.size();
    float opt_acmr = opt_misses / triangle_count;
    float non_opt_acmr = non_opt_misses / triangle_count;
    cout << "[!] Optimized ACMR: " << opt_acmr << " Non: " << non_opt_acmr << endl;
}

unsigned int vcache::getIndexCount() const
{
    return NewTriangleList.size();
}

const unsigned int * vcache::getIndices() const
{
    return &NewTriangleList[0];
}

void vcache::_init_scores()
{
    for (int i=0; i<(int)Verts.size(); ++i)
        _score_vertex(i);

    for (int i=0; i<(int)Tris.size(); ++i)
        _score_triangle(i);
}

void vcache::_init_verts(const MeshBuffer& buffer)
{
    /***************************************
      Find valence counts for all verts
      Make a list to the triangles using them
      init cache positions to -1 (-1 means not added yet)
    ***************************************/
    int size = buffer.getVertCnt();
    Verts.reserve(size);
    for (int i=0; i<size; ++i) {
        Vertex vert;
        vert.cache_pos = -1;
        vert.maxValence = 0;
        vert.score = 0;
        vert.trisNotAdded = 0;
        Verts.push_back(vert);
    }

    const unsigned int *indices = buffer.getIndices();
    for (int i=0; i<buffer.getIdxCnt(); ++i) {
        int vert_idx = indices[i];
        Verts[vert_idx].maxValence++;
        Verts[vert_idx].trisNotAdded++;
        Verts[vert_idx].reference_list.push_back( i / 3 );
    }

    /*
    int average_valence = 0;
    for (int i=0; i<(int)Verts.size(); ++i) {
        average_valence += Verts[i].maxValence;
    }
    average_valence /= Verts.size();
    cout << "[ ] average valence: " << average_valence << endl;
    */
}

void vcache::_init_tris(const MeshBuffer& buffer)
{
    const unsigned int *indices = buffer.getIndices();

    int size = buffer.getIdxCnt() / 3;
    Tris.reserve(size);
    for (int i=0; i<size; ++i) {
        Triangle tri;
        tri.in_cache = false;
        tri.score = 0.0f;
        tri.referenced_verts[0] = indices[i * 3 + 0];
        tri.referenced_verts[1] = indices[i * 3 + 1];
        tri.referenced_verts[2] = indices[i * 3 + 2];
        Tris.push_back(tri);
    }

    NewTriangleList.reserve(buffer.getIdxCnt());
}

void vcache::_score_vertex(int index)
{
    if (Verts[index].trisNotAdded == 0) {
        Verts[index].score = -1.0;
        return;
    }

    float score = 0.0f;
    int cache_pos = Verts[index].cache_pos;
    if (cache_pos < 0) {
        // not in cache, so no score
    }
    else {
        if (cache_pos < 3) {
            // this triangle was used in the last triangle,
            // so it has a fixed score, which ever of the three its in.
            // Otherwise, you can get very different answers
            // depending on whether you add the triangle
            // 1,2,3 or 3,1,2 - which is silly.
            score = LastTriScore;
        }
        else {
            assert( cache_pos < MaxSizeCache );
            const float scalar = 1.0f / (MaxSizeCache - 3);
            score = 1.0f - (cache_pos - 3) * scalar;
            score = powf(score, CacheDecayPower);
        }
    }

    float valenceBoost = powf(Verts[index].trisNotAdded, -ValenceBoostPower);
    score = ValenceBoostScale * valenceBoost;
    Verts[index].score = score;
}

void vcache::_score_triangle(int index)
{
    float score = 0;
    int idx;

    idx = Tris[index].referenced_verts[0];
    score += Verts[idx].score;

    idx = Tris[index].referenced_verts[1];
    score += Verts[idx].score;

    idx = Tris[index].referenced_verts[2];
    score += Verts[idx].score;

    Tris[index].score = score;
}

int  vcache::_find_next_tri()
{
    float highest_score = 0.0f;
    int   highest_index = -1;
    // try searching through the verts that are in the cache to find a triangle
    // that has not been added yet.
    for (int i=0; i<(int)LRU.size(); ++i) {
        int vert_idx = LRU[i];
        for (int j=0; j<(int)Verts[vert_idx].reference_list.size(); ++j) {
            int tri_idx = Verts[vert_idx].reference_list[j];
            if (Tris[tri_idx].in_cache) break;

            _score_triangle(tri_idx);
            if (Tris[tri_idx].score > highest_score) {
                highest_score = Tris[tri_idx].score;
                highest_index = tri_idx;
            }
        }
    }

    if (highest_index == -1) {
        // if the LRU cache is empty, or all of the referenced triangles
        // from the verts have already been in the cache
        // find the highest ranking triangle from all of them
        for (int i=0; i<(int)Tris.size(); ++i) {
            if (Tris[i].in_cache) continue;

            if (Tris[i].score > highest_score) {
                highest_score = Tris[i].score;
                highest_index = i;
            }
        }

    }

    return highest_index;
}

void vcache::_add_tri_to_LRU(int index)
{
    Tris[index].in_cache = true;

    NewTriangleList.push_back(Tris[index].referenced_verts[0]);
    NewTriangleList.push_back(Tris[index].referenced_verts[1]);
    NewTriangleList.push_back(Tris[index].referenced_verts[2]);

    int vert_idx;
    for (int i=0; i<3; ++i) {
        vert_idx = Tris[index].referenced_verts[i];
        Verts[vert_idx].trisNotAdded -= 1;
        if (Verts[vert_idx].trisNotAdded < 0) {
            cerr << "[!] Triangle: " << index << " Vert: " << vert_idx << " has valence less than zero!" << endl;
            Verts[vert_idx].trisNotAdded = 0;
        }

        // now that this triangle has been added to the cache,
        // update the verts by moving the reference to this triangle to the back
        // of the list
        std::vector<int>::iterator iter = Verts[vert_idx].reference_list.begin();
        for (iter; iter!=Verts[vert_idx].reference_list.end(); ++iter ) {
            int tri_idx = *(iter);
            if (tri_idx == index) {
                Verts[vert_idx].reference_list.erase(iter);
                Verts[vert_idx].reference_list.push_back(tri_idx);
                break;
            }
        }

        LRU.push_front(vert_idx);
    }

    while (LRU.size() > (MaxSizeCache-3)) {
        LRU.pop_back();
    }

    for (int i=0; i<(int)LRU.size(); ++i) {
        // score verts
        int vert_idx = LRU[i];
        Verts[vert_idx].cache_pos = i;
        _score_vertex(vert_idx);
    }
}

