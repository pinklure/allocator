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

class MemoryManager {
    PieceNode dummy;

    void memory_alloc(std::size_t init_size) {
        assert(init_size > 0);
        auto ceil_size = ceil_divide(init_size, PAGE_SIZE) * PAGE_SIZE;
        auto cur = &dummy;
        while (cur->next_free != nullptr) {
            cur = cur->next_free;
        }
        cur->next_free = new PieceNode{new char[ceil_size], ceil_size, nullptr, nullptr};

    }

    PieceNode *find_fit_node(std::size_t size) {
        assert(size > 0);
        auto ceil_size = ceil_divide(size, MIN_UNIT) * MIN_UNIT;
        auto cur = &dummy;
        while (cur->next_free != nullptr) {
            cur = cur->next_free;
            if (cur->size >= ceil_size) {
                break;
            }
        }

        if (cur->size < ceil_size) {
            return nullptr;
        } else {
            cur->size -= ceil_size;
            auto node = new PieceNode{cur->data + cur->size, ceil_size, cur->next_free, cur->next_allocated};
            cur->next_allocated = node;
            return node;
        }
    }

    PieceNode *pick_in_allocated(void *data, PieceNode *last_free) {
        auto prev = last_free;
        auto curr = last_free->next_allocated;

        while (curr != nullptr && curr->data != data) {
            prev = curr;
            curr = curr->next_allocated;
        }
        if (curr == nullptr) {
            return nullptr;
        } else {
            prev->next_allocated = curr->next_allocated;
            return curr;
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
    MemoryManager(std::size_t init_size)
            : dummy{nullptr, 0, nullptr, nullptr} {
        if (init_size > 0) {
            memory_alloc(init_size);
        }
    }

    [[nodiscard]] void *alloc(std::size_t sz) {
        if (sz == 0) {
            return nullptr;
        }

        auto node = find_fit_node(sz);
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
        MemoryManager manager(6000);
        manager.visualize();
        auto a = manager.alloc(1231);
        manager.visualize();
        auto b = manager.alloc(123);
        manager.visualize();
        manager.dealloc(a);
        manager.visualize();
        manager.dealloc(b);
        manager.visualize();
    }
};


#endif //ALLOCATOR_MINI_ALLOC_H
