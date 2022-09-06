//
// Created by PinkLure on 9/6/2022.
//

#ifndef ALLOCATOR_CROSS_ALLOC_H
#define ALLOCATOR_CROSS_ALLOC_H

#include"memory_hierachy.h"
#include "3rd/ansi-color.h"

#include <sstream>
#include <iostream>
#include <limits>
#include <vector>

class CrossAlloc {
public:
    CrossAlloc() = delete;

    ~CrossAlloc() = delete;

    void operator=(CrossAlloc const &) = delete;

    static void *alloc(std::size_t size);

    static bool dealloc(void *mem);

    static void print_table(bool free);

    static void print_origin_vec();

    static void visualize();

private:

    struct MemoryNode {
        // managed memory size
        std::size_t size{};

        // managed memory pointer
        std::byte *mem{};

        // list can be free-list or allocated-list
        MemoryNode *list_prev{};
        MemoryNode *list_next{};

        // origin is to keep original memory address sequence for later use
        MemoryNode *origin_prev{};
        MemoryNode *origin_next{};

        // classify node by its size
        Hierachy level{UNDEF};

        // if is in free-list
        bool is_free;

        MemoryNode(Hierachy level, bool is_free)
                : level{level}, is_free{is_free} {};

        MemoryNode(std::size_t size, std::byte *mem, Hierachy level)
                : size{size}, mem{mem}, level{level}, is_free{true} {};

        void insert_after(MemoryNode *node);

        void detach_from_list();

        // return the target-size MemoryNode *
        // if source->size < ceil_size, return nullptr
        // don't touch $source after invoke this func
        // this func will handle node's list relations
        static MemoryNode *divide_node(MemoryNode *source, std::size_t ceil_size);

        // don't touch $node after invoke this func
        // return the merged node or nullptr
        static MemoryNode *merge_neighbors(MemoryNode *node);

    };

    struct OriginNode {
        std::size_t size;
        std::byte *mem;
//        MemoryNode *next;

        explicit OriginNode(std::size_t sz)
                : size{sz}, mem{new std::byte[sz]} {};

    };

private:
    static MemoryNode free_table[Hierachy::SIZE];
    static MemoryNode allocated_table[Hierachy::SIZE];
    inline static std::vector<OriginNode> origin_vec{};

private:
    // request memory from system
    static void request_memory(std::size_t size);

    static void request_memory(Hierachy level);

    // acquire free node by size
    static MemoryNode *acquire_free(std::size_t size);

    // search node by ptr and size in allocated_table
    static MemoryNode *search_allocated(void *mem, std::size_t size);

    // release an allocated node to free, and try to merge neighbors
    static void release_allocated(MemoryNode *node);
};


// ============================ implementation begin =========================================


inline void *CrossAlloc::alloc(std::size_t size) {
    if (size == 0 || size > level2size(Hierachy::G512)) {
        return nullptr;
    }

    auto node = acquire_free(size);
//    if (node == nullptr) {
//        return nullptr;
//    }

    assert(sizeof(std::size_t) == MIN_UNIT);
    *((std::size_t *) node->mem) = node->size;
    return node->mem + sizeof(std::size_t);
}

inline bool CrossAlloc::dealloc(void *mem) {
    if (mem == nullptr) {
        return false;
    }
    auto real_mem = (std::byte *) mem - sizeof(std::size_t);
    auto size = *(std::size_t *) real_mem;

//    auto size = *(std::size_t *) ((std::byte *) mem - sizeof(std::size_t));
    auto node = search_allocated(real_mem, size);
    if (node == nullptr) {
        return false;
    }
    release_allocated(node);
    return true;
}

template<typename UINT>
requires (std::numeric_limits<UINT>::is_integer && !std::numeric_limits<UINT>::is_signed)
inline UINT ceil_divide(UINT x, UINT y) {
    return x == 0 ? 0 : 1 + (x - 1) / y;
}

inline void CrossAlloc::request_memory(std::size_t size) {
    assert(size != 0 && size <= level2size(Hierachy::G512));
    auto ceil_size = ceil_divide(size, PAGE_SIZE) * PAGE_SIZE;
    std::cout << "Alloc Ceiled Size: " << ceil_size << "\n";
    request_memory(size2level_allocate(ceil_size));
}

inline void CrossAlloc::request_memory(Hierachy level) {
    auto real_size = level2size(level);
    origin_vec.emplace_back(real_size);
    auto &back = origin_vec.back();
    auto node = new MemoryNode(real_size, back.mem, level);
//    back.next = node;
    free_table[level].insert_after(node);
}

inline CrossAlloc::MemoryNode *CrossAlloc::acquire_free(std::size_t size) {
    assert(size != 0 && size <= level2size(Hierachy::G512));
    auto ceil_size = (ceil_divide(size, MIN_UNIT) + 1) * MIN_UNIT;
    MemoryNode *node{};
    for (int i = size2level_allocate(ceil_size); i < Hierachy::SIZE; i++) {
        if (free_table[i].list_next != nullptr) {
            node = free_table[i].list_next;
            break;
        }
    }

    if (node == nullptr) {
        auto page_ceil_size = ceil_divide(ceil_size, PAGE_SIZE) * PAGE_SIZE;
        auto page_level = size2level_allocate(page_ceil_size);

        request_memory(page_level);

        node = free_table[page_level].list_next;
    }

    return MemoryNode::divide_node(node, ceil_size);
}

inline CrossAlloc::MemoryNode *CrossAlloc::search_allocated(void *mem, std::size_t size) {
    auto curr = allocated_table[size2level_classify(size)].list_next;
    while (curr != nullptr && curr->mem != mem) {
        curr = curr->list_next;
    }
    return curr;
}

inline void CrossAlloc::release_allocated(MemoryNode *node) {
    node->detach_from_list();
    node->is_free = true;
    auto res = MemoryNode::merge_neighbors(node);
    free_table[res->level].insert_after(res);
}


inline void CrossAlloc::MemoryNode::insert_after(MemoryNode *node) {
    node->list_prev = this;
    node->list_next = this->list_next;
    if (this->list_next != nullptr) {
        this->list_next->list_prev = node;
    }
    this->list_next = node;
}

inline void CrossAlloc::MemoryNode::detach_from_list() {
    if (this->list_prev != nullptr) {
        this->list_prev->list_next = this->list_next;
    }
    if (this->list_next != nullptr) {
        this->list_next->list_prev = this->list_prev;
    }

    this->list_prev = nullptr;
    this->list_next = nullptr;
}

inline CrossAlloc::MemoryNode *CrossAlloc::MemoryNode::divide_node(MemoryNode *source, std::size_t ceil_size) {
    assert(ceil_size != 0 && ceil_size < source->size);
    MemoryNode *res;

    source->detach_from_list();
    if (source->size == ceil_size) {
        source->is_free = false;
        res = source;
    } else {
        source->size -= ceil_size;
        source->level = size2level_classify(source->size);
        free_table[source->level].insert_after(source);

        res = new MemoryNode(ceil_size, source->mem + source->size, size2level_classify(ceil_size));
        res->origin_prev = source;
        res->origin_next = source->origin_next;
        if (source->origin_next != nullptr) {
            source->origin_next->origin_prev = res;
        }
        source->origin_next = res;

        res->is_free = false;
    }

    allocated_table[res->level].insert_after(res);
    return res;
}

inline CrossAlloc::MemoryNode *CrossAlloc::MemoryNode::merge_neighbors(MemoryNode *node) {
    if (node == nullptr) {
        return nullptr;
    }

    node->detach_from_list();

    // find fist free node on origin list
    if (node->origin_prev != nullptr && node->origin_prev->is_free) {
        return merge_neighbors(node->origin_prev);
    }

    if (node->origin_next != nullptr && node->origin_next->is_free) {
        auto next = node->origin_next;
        next->detach_from_list();

        node->size += next->size;
        node->level = size2level_classify(node->size);

        node->origin_next = next->origin_next;
        if (next->origin_next != nullptr) {
            next->origin_next->origin_prev = node;
        }
        delete next;
        return merge_neighbors(node);
    }

    return node;
}


inline CrossAlloc::MemoryNode CrossAlloc::free_table[Hierachy::SIZE] = {
        MemoryNode{B8, true,},
        MemoryNode{B16, true,},
        MemoryNode{B32, true,},
        MemoryNode{B64, true,},
        MemoryNode{B128, true,},
        MemoryNode{B256, true,},
        MemoryNode{B512, true,},
        MemoryNode{K1, true,},
        MemoryNode{K2, true,},
        MemoryNode{K4, true,},
        MemoryNode{K8, true,},
        MemoryNode{K16, true,},
        MemoryNode{K32, true,},
        MemoryNode{K64, true,},
        MemoryNode{K128, true,},
        MemoryNode{K256, true,},
        MemoryNode{K512, true,},
        MemoryNode{M1, true,},
        MemoryNode{M2, true,},
        MemoryNode{M4, true,},
        MemoryNode{M8, true,},
        MemoryNode{M16, true,},
        MemoryNode{M32, true,},
        MemoryNode{M64, true,},
        MemoryNode{M128, true,},
        MemoryNode{M256, true,},
        MemoryNode{M512, true,},
        MemoryNode{G1, true,},
        MemoryNode{G2, true,},
        MemoryNode{G4, true,},
        MemoryNode{G8, true,},
        MemoryNode{G16, true,},
        MemoryNode{G32, true,},
        MemoryNode{G64, true,},
        MemoryNode{G128, true,},
        MemoryNode{G256, true,},
        MemoryNode{G512, true,},
};

inline CrossAlloc::MemoryNode CrossAlloc::allocated_table[Hierachy::SIZE] = {
        MemoryNode{B8, false,},
        MemoryNode{B16, false,},
        MemoryNode{B32, false,},
        MemoryNode{B64, false,},
        MemoryNode{B128, false,},
        MemoryNode{B256, false,},
        MemoryNode{B512, false,},
        MemoryNode{K1, false,},
        MemoryNode{K2, false,},
        MemoryNode{K4, false,},
        MemoryNode{K8, false,},
        MemoryNode{K16, false,},
        MemoryNode{K32, false,},
        MemoryNode{K64, false,},
        MemoryNode{K128, false,},
        MemoryNode{K256, false,},
        MemoryNode{K512, false,},
        MemoryNode{M1, false,},
        MemoryNode{M2, false,},
        MemoryNode{M4, false,},
        MemoryNode{M8, false,},
        MemoryNode{M16, false,},
        MemoryNode{M32, false,},
        MemoryNode{M64, false,},
        MemoryNode{M128, false,},
        MemoryNode{M256, false,},
        MemoryNode{M512, false,},
        MemoryNode{G1, false,},
        MemoryNode{G2, false,},
        MemoryNode{G4, false,},
        MemoryNode{G8, false,},
        MemoryNode{G16, false,},
        MemoryNode{G32, false,},
        MemoryNode{G64, false,},
        MemoryNode{G128, false,},
        MemoryNode{G256, false,},
        MemoryNode{G512, false,},
};


void CrossAlloc::print_table(bool free) {
    using namespace std::string_literals;
    std::string res{};
    static auto const free_label = AnsiColor::colorize<AnsiColor::GREEN>("[Free  ]");
    static auto const allocated_label = AnsiColor::colorize<AnsiColor::RED>("[Alloc ]");

    std::stringstream ss{};

    MemoryNode *table = free ? free_table : allocated_table;

//    for (auto &i: free_table) {
    for (int i = 0; i < Hierachy::SIZE; i++) {
        auto curr = table[i].list_next;
        while (curr != nullptr) {
            if (free) {
                assert(curr->is_free);
                res += free_label + " ";
            } else {
                assert(!curr->is_free);
                res += allocated_label + " ";
            }

            ss.str("");
            ss << "[" << (void *) curr->mem << ", " << (void *) (curr->mem + curr->size) << "]";
            res += AnsiColor::colorize<AnsiColor::BLUE>(ss.str()) + " ";

            ss.str("");
            ss << "[" << level2str(curr->level) << "]";
            res += AnsiColor::colorize<AnsiColor::MAGENTA>(ss.str()) + " ";

            ss.str("");
            ss << "size: " << curr->size;
            res += AnsiColor::colorize<AnsiColor::CYAN>(ss.str()) + "\n";

            curr = curr->list_next;
        }
    }
    std::cout << res;
    std::cout << std::flush;
}

void CrossAlloc::print_origin_vec() {
    using namespace std::string_literals;
    std::string res{};
    auto label = AnsiColor::colorize<AnsiColor::YELLOW>("[Origin]") + " ";
    std::stringstream ss{};
    for (auto &it: origin_vec) {
        res += label;

        ss.str("");
        ss << "[" << (void *) it.mem << ", " << (void *) (it.mem + it.size) << "]";
        res += AnsiColor::colorize<AnsiColor::BLUE>(ss.str()) + " ";

        ss.str("");
        ss << "[" << level2str(size2level_classify(it.size)) << "]";
        res += AnsiColor::colorize<AnsiColor::MAGENTA>(ss.str()) + " ";

        ss.str("");
        ss << "size: " << it.size;
        res += AnsiColor::colorize<AnsiColor::CYAN>(ss.str()) + "\n";
    }

    std::cout << res;
    std::cout << std::flush;
}

void CrossAlloc::visualize() {
    std::cout << "=========================VISUALIZE===============================\n";
    print_origin_vec();
    print_table(true); // free list
    print_table(false); // allocated list
    std::cout << "============================END==================================\n";
}


#endif //ALLOCATOR_CROSS_ALLOC_H
