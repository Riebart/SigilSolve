#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <atomic>

#define NUM_BLOCK_TYPES 7
#define MAX_BLOCK_TYPE_SIZE 6

// Convert coordinates to an index
#define BLOCK_COORD(block, x, y) (y*block.width+x)

std::atomic<uint64_t> num_configs;
bool solution_found = false;

// Block shapes always contain an array of "height" number of rows, each "width" items long.
struct block
{
    int width;
    int height;
    int** shape;
};

struct coord
{
    int x;
    int y;
};

struct block* block_types[4][NUM_BLOCK_TYPES];

void print_block_shape(struct block b)
{
    for (int y = 0; y < b.height; y++)
    {
        for (int x = 0; x < b.width; x++)
        {
            printf((b.shape[y][x] == 1 ? "#" : " "));
        }
        printf("\n");
    }
}

void print_board(struct block b, int offset = 0)
{
    for (int y = 0; y < b.height; y++)
    {
        for (int x = 0; x < b.width; x++)
        {
            if (b.shape[y][x] == 0)
            {
                printf("-");
            }
            else
            {
                if (offset == 0)
                {
                    printf("%c", 'A' + b.shape[y][x] - 1);
                }
                else
                {
                    printf("%c", 'A' + offset - b.shape[y][x]);
                }
            }
        }
        printf("\n");
    }
}

bool board_filled(struct block* board)
{
    for (int x = 0; x < board->width; x++)
    {
        for (int y = 0; y < board->height; y++)
        {
            if (board->shape[y][x] == 0)
            {
                return false;
            }
        }
    }
    return true;
}

struct coord* board_coords(int width, int height)
{
    struct coord* coords = new struct coord[width * height];
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            coords[y*width + x].x = x;
            coords[y*width + x].y = y;
        }
    }

    return coords;
}

void usage()
{
    printf("Usage: SigilSolve <width> <height> <block ID> ...\n");
    printf("Available blocks are...\n\n");

    for (int i = 0; i < NUM_BLOCK_TYPES; i++)
    {
        printf("Block ID: %d\n", i);
        print_block_shape(*block_types[0][i]);
        printf("\n");
    }
}

void rotate_right(struct block* b, struct block* rb)
{
    for (int i = 0; i < b->width; i++)
    {
        for (int j = 0; j < b->height; j++)
        {
            rb->shape[i][b->height - j - 1] = b->shape[j][i];
        }
    }
}

void add_block(struct block* board, struct block* block, int x, int y, int scale = 1)
{
    for (int i = 0; i < block->width; i++)
    {
        for (int j = 0; j < block->height; j++)
        {
            board->shape[j + y][i + x] += scale * block->shape[j][i];
        }
    }
}

void remove_block(struct block* board, struct block* block, int x, int y, int scale = 1)
{
    add_block(board, block, x, y, -scale);
}

bool test_position(struct block* board, struct block* block, int x, int y)
{
    for (int i = 0; i < block->width; i++)
    {
        for (int j = 0; j < block->height; j++)
        {
            if ((board->shape[j + y][i + x] * block->shape[j][i]) > 0)
            {
                return false;
            }
        }
    }

    return true;
}

struct block* make_block(int width, int height, int vals[])
{
    int pos = 0;

    struct block* b = (struct block*)malloc(sizeof(struct block));
    b->width = width;
    b->height = height;

    b->shape = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++)
    {
        b->shape[i] = (int*)malloc(width * sizeof(int));
        if (vals != NULL)
        {
            for (int x = 0; x < width; x++)
            {
                b->shape[i][x] = vals[pos + x];
            }
            pos += width;
        }
        else
        {
            for (int x = 0; x < width; x++)
            {
                b->shape[i][x] = 0;
            }
        }
    }

    return b;
}

void destroy_block(struct block* b)
{
    for (int i = 0; i < b->height; i++)
    {
        free(b->shape[i]);
    }
    free(b->shape);
    free(b);
}

bool compare_block(struct block* b1, struct block* b2)
{
    if ((b1->width != b2->width) || (b1->height != b2->height))
    {
        return false;
    }

    for (int y = 0; y < b1->height; y++)
    {
        for (int x = 0; x < b1->width; x++)
        {
            if (b1->shape[y][x] != b2->shape[y][x])
            {
                return false;
            }
        }
    }

    return true;
}

bool place_block(struct block** boards, struct coord* coords, int* block_ids, int num_blocks, bool is_top = true)
{
    int id = block_ids[0];
    bool ret = false;

    for (int r = 0; r < 4; r++)
    {
        if ((block_types[r][id] == NULL) || solution_found)
        {
            break;
        }

#pragma omp parallel for firstprivate(block_ids, num_blocks)
        for (int ci = 0 ; ci < boards[0]->width * boards[0]->height ; ci++)
        {
            int x = coords[ci].x;
            int y = coords[ci].y;
            int thread_count = omp_get_num_threads();
            // Inside of a recursed section, the thread count will always be 1 as it won't
            // exponentially parallelized, so in that case take the first (only) board,
            // otherwise take the board that corresponds to the starting position.
            struct block* board = boards[is_top ? ci : 0];

            // If the block width in this rotation would exceed the board width, then
            // stop trying this, or any other, horizontal positions.
            if (((x + block_types[r][id]->width) > board->width) || solution_found)
            {
                continue;
            }

            if (((y + block_types[r][id]->height) > board->height) || solution_found)
            {
                continue;
            }

            if (test_position(board, block_types[r][id], x, y))
            {
                add_block(board, block_types[r][id], x, y, num_blocks);
                num_configs.fetch_add(1);

                // If we found a working position, and this is the last block
                if (num_blocks == 1)
                {
                    ret = true;
                }
                // Otherwise, there's more than one block, and we should recurse
                else
                {
                    // Because the recursed sections won't parallelize, they will all have 
                    // a therad ID of 0, so when recursing, the boards list needs to be exactly
                    // one item: the board for this thread.
                    bool child = place_block(&board, coords, block_ids + 1, num_blocks - 1, false);
                    if (child)
                    {
                        ret = true;
                    }
                    else
                    {
                        remove_block(board, block_types[r][id], x, y, num_blocks);
                    }
                }
            }
        }
    }

    if (ret)
    {
        solution_found = true;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int block_type_shapes[][MAX_BLOCK_TYPE_SIZE] = {
            { 1, 1, 0, 0, 1, 1 },
            { 0, 1, 1, 1, 1, 0 },
            { 1, 0, 1, 0, 1, 1 },
            { 0, 1, 0, 1, 1, 1 },
            { 0, 1, 0, 1, 1, 1 },
            { 1, 1, 1, 1, 0, 0 },
            { 1, 1, 1, 1, 0, 0 }
    };

    int block_type_sizes[][2] = {
            { 3, 2 },
            { 3, 2 },
            { 2, 3 },
            { 2, 3 },
            { 3, 2 },
            { 2, 2 },
            { 4, 1 }
    };

    for (int i = 0; i < NUM_BLOCK_TYPES; i++)
    {
        block_types[0][i] = make_block(block_type_sizes[i][0], block_type_sizes[i][1], block_type_shapes[i]);

        for (int r = 1; r < 4; r++)
        {
            block_types[r][i] = make_block(block_type_sizes[i][r % 2], block_type_sizes[i][(r + 1) % 2], block_type_shapes[i]);
            rotate_right(block_types[r - 1][i], block_types[r][i]);

            if (compare_block(block_types[0][i], block_types[r][i]))
            {
                destroy_block(block_types[r][i]);
                block_types[r][i] = NULL;
                break;
            }
        }
    }

    if (argc < 4)
    {
        usage();
        return 1;
    }

    int boardW, boardH;
    sscanf(argv[1], "%d", &boardW);
    sscanf(argv[2], "%d", &boardH);

    int num_blocks = argc - 3;
    int* block_ids = (int*)malloc(sizeof(int) * num_blocks);
    int s = 0;
    for (int i = 3; i < argc; i++)
    {
        sscanf(argv[i], "%d", &block_ids[i - 3]);
        for (int j = 0; j < MAX_BLOCK_TYPE_SIZE; j++)
        {
            s += block_type_shapes[block_ids[i - 3]][j];
        }
    }

    if (s != (boardW * boardH))
    {
        printf("Chosen blocks (%d squares) won't fill the board (%d squares).", s, boardW * boardH);
        return 2;
    }

    printf("Detected %d OMP processors\n", omp_get_num_procs());
    int nthreads = omp_get_max_threads();
    printf("Maximum of %d OMP threads\n", nthreads);

    // In serial sections, the number of threads is always 1.
    // SO to get a real indicator of how many threads will be used, spin up a parallel section
    // and print out, in a master only section, how many threads are in use. This gets the right
    // number printed only once.
#pragma omp parallel
    {
#pragma omp master
        {
            nthreads = omp_get_num_threads();
        }
    }

    printf("Parallel sections will use %d OMP threads\n", nthreads);
    printf("\n");

    // Create one starting board per thread loop variable (that is one per board square).
    struct coord* coords = board_coords(boardW, boardH);
    struct block** boards = new block*[boardW * boardH];
    for (int i = 0; i < (boardW * boardH); i++)
    {
        boards[i] = make_block(boardW, boardH, NULL);
    }
    
    num_configs = 0;
    place_block(boards, coords, block_ids, num_blocks);
    
    for (int i = 0; i < (boardW * boardH); i++)
    {
        if (board_filled(boards[i]))
        {
            print_board(*boards[i], num_blocks);
            printf("\n");
        }
    }

    free(block_ids);
    printf("%lu configurations searched\n", num_configs.load());
    return 0;
}