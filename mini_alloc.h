//
// Created by PinkLure on 9/4/2022.
//

#ifndef ALLOCATOR_MINI_ALLOC_H
#define ALLOCATOR_MINI_ALLOC_H

#include "3rd/ansi-color.h"
#include <cassert>
#include <sstream>
#include <iostream>
#include <cstddef>
#include <limits>

constexpr auto PAGE_SIZE = 4096UL;
constexpr auto MIN_UNIT = 8UL; // 8 * n

template<typename UINT>
requires (std::numeric_limits<UINT>::is_integer && !std::numeric_limits<UINT>::is_signed)
UINT ceil_divide(UINT x, UINT y) {
    return x == 0 ? 0 : 1 + (x - 1) / y;
}

struct PieceNode {
    char *data;
    std::size_t size;
    PieceNode *next_free; // nullptr if this is an allocated node
    PieceNode *next_allocated;
};

class MiniAlloc {
    PieceNode dummy;

    void memory_alloc(std::size_t init_size) {
        assert(init_size > 0);
        auto ceil_size = ceil_divide(init_size, PAGE_SIZE) * PAGE_SIZE;
        auto node = new PieceNode{new char[ceil_size], ceil_size, dummy.next_free, dummy.next_allocated};
        dummy.next_free = node;
    }

    PieceNode *pick_fit_node(std::size_t size) {
        assert(size > 0);
        auto ceil_size = ceil_divide(size, MIN_UNIT) * MIN_UNIT;
        auto prev = &dummy;
        auto curr = prev->next_free;

        while (curr != nullptr && curr->size < size) {
            prev = curr;
            curr = prev->next_free;
        }

        if (curr == nullptr) {
            memory_alloc(ceil_size);
            return pick_fit_node(ceil_size);
        } else {
            if (curr->size == ceil_size) {
                prev->next_free = curr->next_free;
                curr->next_free = nullptr;
                prev->next_allocated = curr;
                return curr;
            } else {
                auto node = new PieceNode{curr->data + curr->size - ceil_size, ceil_size, nullptr,
                                          curr->next_allocated};
                curr->size -= ceil_size;
                curr->next_allocated = node;
                return node;
            }
        }
    }

    static PieceNode *pick_in_allocated(void *data, PieceNode *last_free) {
        auto prev = last_free;
        auto curr = last_free->next_allocated;
        while (curr != nullptr && curr->data != data) {
            prev = curr;
            curr = curr->next_allocated;
        }

        if (curr == nullptr) {
            return nullptr;
        } else {
            prev->next_allocated = nullptr;
            return curr;
        }
    }

    static void try_merge(PieceNode *node1, PieceNode *node2) {
        if (node1 == nullptr || node2 == nullptr) {
            return;
        }
        if (node1->data + node1->size == node2->data) {
            // no need to check if node1->next_allocated == nullptr
            node1->next_free = node2->next_free;
            node1->next_allocated = node2->next_allocated;
            node1->size += node2->size;
            delete node2;
        }
    }


    void do_dealloc(void *data) {
        auto curr = &dummy;
        while (curr != nullptr) {
            if (curr->next_allocated != nullptr) {
                auto res = pick_in_allocated(data, curr);
                if (res != nullptr) {
                    res->next_free = curr->next_free;
                    curr->next_free = res;
                    try_merge(curr, res);
                    try_merge(res, res->next_free);
                    return;
                }
            }
            curr = curr->next_free;
        }
    }

    void do_print_free(PieceNode *node) {
        if (node == nullptr) {
            return;
        }

        std::stringstream ss{};
        auto status = AnsiColor::colorize<AnsiColor::GREEN, AnsiColor::BLACK>("[  Free   ]");
        ss << "[" << (void *) node->data << ", " << (void *) (node->data + node->size) << "]";
        auto address_range = AnsiColor::colorize<AnsiColor::BLUE>(ss.str()) + ", ";
        ss.str("");
        ss << "size: " << node->size;
        auto size = AnsiColor::colorize<AnsiColor::YELLOW>(ss.str()) + ", ";
        ss.str("");

        ss << "Next Free: " << (void *) node->next_free;
        auto next_free = AnsiColor::colorize<AnsiColor::MAGENTA>(ss.str()) + ", ";
        ss.str("");

        ss << "Next Allocated: " << (void *) node->next_allocated;
        auto next_allocated = AnsiColor::colorize<AnsiColor::CYAN>(ss.str()) + ";";
        ss.str("");

        std::cout << status << address_range << size << next_free << next_allocated << "\n";

        do_print_allocated(node->next_allocated);
        do_print_free(node->next_free);
    }

    void do_print_allocated(PieceNode *node) {
        if (node == nullptr) {
            return;
        }
        std::stringstream ss{};
        auto status = AnsiColor::colorize<AnsiColor::RED, AnsiColor::BLACK>("[Allocated]");
        ss << "[" << (void *) node->data << ", " << (void *) (node->data + node->size) << "]";
        auto address_range = AnsiColor::colorize<AnsiColor::BLUE>(ss.str()) + ", ";
        ss.str("");
        ss << "size: " << node->size;
        auto size = AnsiColor::colorize<AnsiColor::YELLOW>(ss.str()) + ", ";
        ss.str("");

        ss << "Next Free: " << (void *) node->next_free;
        auto next_free = AnsiColor::colorize<AnsiColor::MAGENTA>(ss.str()) + ", ";
        ss.str("");

        ss << "Next Allocated: " << (void *) node->next_allocated;
        auto next_allocated = AnsiColor::colorize<AnsiColor::CYAN>(ss.str()) + ";";
        ss.str("");

        std::cout << status << address_range << size << next_free << next_allocated << "\n";
        do_print_allocated(node->next_allocated);
    }


public:
    explicit MiniAlloc(std::size_t init_size)
            : dummy{nullptr, 0, nullptr, nullptr} {
        if (init_size > 0) {
            memory_alloc(init_size);
        }
    }

    [[nodiscard]] void *alloc(std::size_t sz) {
        if (sz == 0) {
            return nullptr;
        }

        auto node = pick_fit_node(sz);
        return node == nullptr ? nullptr : node->data;
    }

    void dealloc(void *data) {
        do_dealloc(data);
    }


    void visualize() {
        std::cout << "==================Begin Visualize Memory Information==================\n";
        do_print_free(&dummy);
        std::cout << "===================End Visualize Memory Information==================\n\n";
    }


    static void test() {
#define Init(initsize) MiniAlloc manager(initsize); manager.visualize()
#define Alloc(size) manager.alloc(size); manager.visualize()
#define Free(data) manager.dealloc(data); manager.visualize()

        Init(6000);

        auto b = Alloc(331);
        auto a = Alloc(124);
        Free(a);
        auto e = Alloc(1025);
        auto c = Alloc(854);
        auto d = Alloc(532);
        Free(d);
        Free(e);
        Free(b);

    }
};


#endif //ALLOCATOR_MINI_ALLOC_H
