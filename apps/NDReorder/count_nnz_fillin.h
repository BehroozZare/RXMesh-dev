#pragma once

#include "rxmesh/matrix/sparse_matrix.h"

#include <Eigen/Sparse>

template <typename SparseMatrixType>
void save_sparse_mat(const SparseMatrixType& mat,
                     const std::string&      filename,
                     int                     base = 0)
{
    // base = 1: 1-based indexing for MATLAB
    //  base = 0: 0-based indexing for MATLAB

    std::ofstream file(filename);
    if (!file.is_open()) {
        RXMESH_ERROR("Error opening file: {}", filename);
        return;
    }

    for (int k = 0; k < mat.outerSize(); ++k) {
        for (typename SparseMatrixType::InnerIterator it(mat, k); it; ++it) {
            file << (it.row() + base) << " " << (it.col() + base) << " "
                 << it.value() << std::endl;
        }
    }

    file.close();
}

void save_permutation(const std::vector<int>& h_permute,
                      const std::string&      filename)
{

    std::ofstream file(filename);
    if (!file.is_open()) {
        RXMESH_ERROR("Error opening file: {}", filename);
        return;
    }

    for (int k = 0; k < h_permute.size(); ++k) {
        file << h_permute[k] << "\n";
    }

    file.close();
}

/**
 * @brief calculate the total number of nnz after Cholesky factorization given a
 * permutation array that will be applied before the factorization
 */
template <typename EigeMatT, typename I>
int count_nnz_fillin(const EigeMatT& eigen_mat,
                     std::vector<I>& h_permute,
                     std::string     st = "")
{
    using namespace rxmesh;

    assert(h_permute.size() == eigen_mat.rows());


    // std::cout << "eigen_mat\n" << eigen_mat << "\n";

    // permutation matrix
    Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> perm(
        eigen_mat.rows());
    for (int i = 0; i < eigen_mat.rows(); ++i) {
        perm.indices()[i] = h_permute[i];
    }

    Eigen::SparseMatrix<float> permuted_mat(eigen_mat.rows(), eigen_mat.rows());

    Eigen::internal::permute_symm_to_fullsymm<Eigen::Lower, false>(
        eigen_mat, permuted_mat, perm.indices().data());


    // save_sparse_mat(permuted_mat,
    //                 std::string("C:\\Github\\Matlab_Reordering_Trial\\") +
    //     st + std::string(".txt"),1);

    // compute Cholesky factorization on the permuted matrix

    Eigen::SimplicialLLT<Eigen::SparseMatrix<float>,
                         Eigen::Lower,
                         Eigen::NaturalOrdering<int>>
        solver;

    solver.analyzePattern(permuted_mat);
    solver.compute(permuted_mat);


    if (solver.info() != Eigen::Success) {
        RXMESH_ERROR(
            "post_chol_factorization_nnz(): Cholesky decomposition with "
            "reorder failed with code {}",
            solver.info());
        return -1;
    }

    int nnz = solver.matrixL().nestedExpression().nonZeros();
    return nnz;

    //// extract nnz from lower matrix
    // Eigen::SparseMatrix<float> lower_mat = solver.matrixL();
    //
    //// these are the nnz on (strictly) the lower part
    // int lower_nnz = lower_mat.nonZeros() - lower_mat.rows();
    //
    //// multiply by two to account for lower and upper parts of the matrix
    //// add rows() to account for entries along the diagonal
    // return 2 * lower_nnz + lower_mat.rows();
}

/**
 * @brief compute the number of nnz that will result if we compute Cholesky
 * decomposition on an input matrix. Taken from
 * Eigen::SimplicialCholeskyBase::analyzePattern_preordered
 */
template <typename T>
int count_nnz_fillin(const rxmesh::SparseMatrix<T>& mat)
{
    const int size = mat.rows();

    std::vector<int> parent(size);
    std::vector<int> nonZerosPerCol(size);
    std::vector<int> tags(size);
    int              nnz = 0;

    for (int r = 0; r < size; ++r) {
        /* L(r,:) pattern: all nodes reachable in etree from nz in A(0:r-1,r) */
        parent[r]         = -1; /* parent of r is not yet known */
        tags[r]           = r;  /* mark node r as visited */
        nonZerosPerCol[r] = 0;  /* count of nonzeros in column r of L */

        int start = mat.row_ptr()[r];
        int end   = mat.row_ptr()[r + 1];

        for (int i = start; i < end; ++i) {
            int c = mat.col_idx()[i];

            if (c < r) {
                /* follow path from c to root of etree, stop at flagged node */
                for (; tags[c] != r; c = parent[c]) {
                    /* find parent of c if not yet determined */
                    if (parent[c] == -1)
                        parent[c] = r;
                    nonZerosPerCol[c]++; /* L (r,c) is nonzero */
                    nnz++;
                    tags[c] = r; /* mark c as visited */
                }
            }
        }
    }

    // multiply by two to account for lower and upper parts of the matirx
    // add rows() to account for entries along the diagonal
    return 2 * nnz + mat.rows();
}