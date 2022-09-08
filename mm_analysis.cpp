#include <iostream>
#include <vector>
#include <random>
#include <time.h>
#include <sys/time.h>
using namespace std;

typedef vector<vector<double>> matrix;

#define RAND_LOWER_BOUND 1
#define RAND_UPPER_BOUND 2

#define N_A_ROW 800
#define N_A_COL_B_ROW 800
#define N_B_COL 800

int A_row = N_A_ROW, A_col = N_A_COL_B_ROW, B_row = N_A_COL_B_ROW, B_col = N_B_COL;

long long wall_clock_time()
{
#ifdef LINUX
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
#endif
}

// A: row-wise read
// B: column-wise read
// C: row-wise write
void mm_ijk(matrix& A, matrix& B, matrix& C) {
    for (int i = 0; i < A_row; ++i) {
        for (int j = 0; j < B_col; ++j) {
            double sum = 0;
            for (int k = 0; k < B_row; ++k) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// A: row-wise read
// B: column-wise read
// C: column-wise write
void mm_jik(matrix& A, matrix& B, matrix& C) {
    for (int j = 0; j < B_col; ++j) {
        for (int i = 0; i < A_row; ++i) {
            double sum = 0;
            for (int k = 0; k < B_row; ++k) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// A: row-wise read
// B: row-wise read
// C: row-wise write
void mm_ikj(matrix& A, matrix& B, matrix& C) {
    for (int i = 0; i < A_row; ++i) {
        for (int k = 0; k < B_row; ++k) {
            double t = A[i][k];
            for (int j = 0; j < B_col; ++j) {
                C[i][j] += t * B[k][j];
            }
        }
    }
}

// A: column-wise read
// B: row-wise read
// C: row-wise write
void mm_kij(matrix& A, matrix& B, matrix& C) {
    for (int k = 0; k < B_row; ++k) {
        for (int i = 0; i < A_row; ++i) {
            double t = A[i][k];
            for (int j = 0; j < B_col; ++j) {
                C[i][j] += t * B[k][j];
            }
        }
    }
}

// A: column-wise read
// B: row-wise read
// C: column-wise write
void mm_kji(matrix& A, matrix& B, matrix& C) {
    for (int k = 0; k < B_row; ++k) {
        for (int j = 0; j < B_col; ++j) {
            double t = B[k][j];
            for (int i = 0; i < A_row; ++i) {
                C[i][j] += A[i][k] * t;
            }
        }
    }
}

// A: column-wise read
// B: column-wise read
// result: column-wise write
void mm_jki(matrix& A, matrix& B, matrix& C) {
    for (int j = 0; j < B_col; ++j) {
        for (int k = 0; k < B_row; ++k) {
            double t = B[k][j];
            for (int i = 0; i < A_row; ++i) {
                C[i][j] += A[i][k] * t;
            }
        }
    }
}

/* Efficiency: ikj > kij ≈ ijk > jik ≈ kji > jki */
/* Paractical: ikj > kij > jik > ijk > kji > jki */
/* Why jik is faster than ijk ??? It's really strange... */

void init_matrix(matrix& m, int row, int col) {
    double r = RAND_UPPER_BOUND - RAND_LOWER_BOUND;

    for (int i = 0; i < row; ++i) {
        for (int j = 0; j < col; ++j) {
            m[i][j] = (rand() / double(RAND_MAX)) * r + RAND_LOWER_BOUND;
        }
    }
}

void output_matrix(string info, matrix& m) {
    cout<<info<<endl;
    for (int i = 0; i < m.size(); ++i) {
        for (int j = 0; j < m[i].size(); ++j) {
            cout<<m[i][j]<<" ";
        }
        cout<<endl;
    }
    cout<<endl;
}

void clear_matrix(matrix& m) {
    for (int i = 0; i < m.size(); ++i) {
        for (int j = 0; j < m[i].size(); ++j) {
            m[i][j] = 0;
        }
    }
}

void work(string title, matrix& A, matrix& B, matrix& res, void(*func)(matrix&, matrix&, matrix&)) {
    long long before, after;

    before = wall_clock_time();
    func(A, B, res);
    after = wall_clock_time();
    printf("%s (%d x %d) x (%d x %d), matrix multiplication took %f seconds\n", title.c_str(), A_row, A_col, B_row, B_col, ((float)(after - before)) / 1000000000);

//    output_matrix("mm_ijk", res);     // debug
    clear_matrix(res);
}

int main() {
    /* correctness test */
//    matrix A = {
//            {1.1, 1.2, 1.3},
//            {2.1, 2.2, 2.3}
//    };
//    matrix B = {
//            {1.1, 1.2},
//            {2.1, 2.2},
//            {3.1, 3.2}
//    };
//    matrix res(A.size(), vector<double>(B[0].size()));

    matrix A(A_row, vector<double>(A_col));
    matrix B(B_row, vector<double>(B_col));
    matrix res(A_row, vector<double>(B_col));


    init_matrix(A, A_row, A_col);
    init_matrix(B, B_row, B_col);

    work("mm_ijk", A, B, res, mm_ijk);
    work("mm_jik", A, B, res, mm_jik);
    work("mm_ikj", A, B, res, mm_ikj);
    work("mm_kij", A, B, res, mm_kij);
    work("mm_kji", A, B, res, mm_kji);
    work("mm_jki", A, B, res, mm_jki);

    return 0;
}
