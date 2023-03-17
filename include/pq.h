// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "utils.h"

#define NUM_PQ_BITS 8
#define NUM_PQ_CENTROIDS (1 << NUM_PQ_BITS)
#define MAX_OPQ_ITERS 20
#define NUM_KMEANS_REPS_PQ 12
#define MAX_PQ_TRAINING_SET_SIZE 256000
#define MAX_PQ_CHUNKS 384

namespace diskann
{
class FixedChunkPQTable
{
    float *tables = nullptr; // pq_tables = float array of size [256 * ndims]
    _u64 ndims = 0;          // ndims = true dimension of vectors
    _u64 n_chunks = 0;
    bool use_rotation = false;
    _u32 *chunk_offsets = nullptr;
    float *centroid = nullptr;
    float *tables_tr = nullptr; // same as pq_tables, but col-major
    float *rotmat_tr = nullptr;

  public:
    FixedChunkPQTable();

    virtual ~FixedChunkPQTable();

#ifdef EXEC_ENV_OLS
    void load_pq_centroid_bin(MemoryMappedFiles &files, const char *pq_table_file, size_t num_chunks);
#else
    void load_pq_centroid_bin(const char *pq_table_file, size_t num_chunks);
#endif

    _u32 get_num_chunks();

    void preprocess_query(float *query_vec);

    // assumes pre-processed query
    void populate_chunk_distances(const float *query_vec, float *dist_vec);

    float l2_distance(const float *query_vec, _u8 *base_vec);

    float inner_product(const float *query_vec, _u8 *base_vec);

    // assumes no rotation is involved
    void inflate_vector(_u8 *base_vec, float *out_vec);

    void populate_chunk_inner_products(const float *query_vec, float *dist_vec);
};

template <typename T> struct PQScratch
{
    float *aligned_pqtable_dist_scratch = nullptr; // MUST BE AT LEAST [256 * NCHUNKS]
    float *aligned_dist_scratch = nullptr;         // MUST BE AT LEAST diskann MAX_DEGREE
    _u8 *aligned_pq_coord_scratch = nullptr;       // MUST BE AT LEAST  [N_CHUNKS * MAX_DEGREE]
    float *rotated_query = nullptr;
    float *aligned_query_float = nullptr;

    PQScratch(size_t graph_degree, size_t aligned_dim)
    {
        diskann::alloc_aligned((void **)&aligned_pq_coord_scratch,
                               (_u64)graph_degree * (_u64)MAX_PQ_CHUNKS * sizeof(_u8), 256);
        diskann::alloc_aligned((void **)&aligned_pqtable_dist_scratch, 256 * (_u64)MAX_PQ_CHUNKS * sizeof(float), 256);
        diskann::alloc_aligned((void **)&aligned_dist_scratch, (_u64)graph_degree * sizeof(float), 256);
        diskann::alloc_aligned((void **)&aligned_query_float, aligned_dim * sizeof(float), 8 * sizeof(float));
        diskann::alloc_aligned((void **)&rotated_query, aligned_dim * sizeof(float), 8 * sizeof(float));

        memset(aligned_query_float, 0, aligned_dim * sizeof(float));
        memset(rotated_query, 0, aligned_dim * sizeof(float));
    }

    void set(size_t dim, T *query, const float norm = 1.0f)
    {
        for (size_t d = 0; d < dim; ++d)
        {
            if (norm != 1.0f)
                rotated_query[d] = aligned_query_float[d] = static_cast<float>(query[d]) / norm;
            else
                rotated_query[d] = aligned_query_float[d] = static_cast<float>(query[d]);
        }
    }
};

void aggregate_coords(const std::vector<unsigned> &ids, const _u8 *all_coords, const _u64 ndims, _u8 *out);

void pq_dist_lookup(const _u8 *pq_ids, const _u64 n_pts, const _u64 pq_nchunks, const float *pq_dists,
                    std::vector<float> &dists_out);

// Need to replace calls to these with calls to vector& based functions above
void aggregate_coords(const unsigned *ids, const _u64 n_ids, const _u8 *all_coords, const _u64 ndims, _u8 *out);

void pq_dist_lookup(const _u8 *pq_ids, const _u64 n_pts, const _u64 pq_nchunks, const float *pq_dists,
                    float *dists_out);

DISKANN_DLLEXPORT int generate_pq_pivots(const float *const train_data, size_t num_train, unsigned dim,
                                         unsigned num_centers, unsigned num_pq_chunks, unsigned max_k_means_reps,
                                         std::string pq_pivots_path, bool make_zero_mean = false);

DISKANN_DLLEXPORT int generate_opq_pivots(const float *train_data, size_t num_train, unsigned dim, unsigned num_centers,
                                          unsigned num_pq_chunks, std::string opq_pivots_path,
                                          bool make_zero_mean = false);

template <typename T>
int generate_pq_data_from_pivots(const std::string data_file, unsigned num_centers, unsigned num_pq_chunks,
                                 std::string pq_pivots_path, std::string pq_compressed_vectors_path,
                                 bool use_opq = false);

template <typename T>
void generate_disk_quantized_data(const std::string data_file_to_use, const std::string disk_pq_pivots_path,
                                  const std::string disk_pq_compressed_vectors_path,
                                  const diskann::Metric compareMetric, const double p_val, size_t &disk_pq_dims);

template <typename T>
void generate_quantized_data(const std::string data_file_to_use, const std::string pq_pivots_path,
                             const std::string pq_compressed_vectors_path, const diskann::Metric compareMetric,
                             const double p_val, const size_t num_pq_chunks, const bool use_opq, const std::string codebook_path);
} // namespace diskann
