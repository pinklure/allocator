//
// Created by PinkLure on 9/4/2022.
//

#ifndef ANSI_COLOR_ANSI_COLOR_H
#define ANSI_COLOR_ANSI_COLOR_H

#ifdef DEBUG

#include <iostream>

#endif

#include <string>
#include <string_view>


class AnsiColor {
public:
    AnsiColor() = delete;

    ~AnsiColor() = delete;

    void operator=(AnsiColor const &) = delete;

    enum Color {
        BLACK,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE,
        SIZE,
    };

    template<Color Front = Color::WHITE, Color Back = Color::BLACK>
    static std::string
    colorize(std::string_view view, bool bright = false, bool underline = false, bool inverse = false) {
        static constexpr auto HEAD = "\033[";
        static constexpr auto TAIL = "\033[0m";
        std::string res{HEAD};
        using namespace std::string_literals;
        if (bright) {
            res += Setting::BRIGHT_ON + ";"s;
        }
        if (underline) {
            res += Setting::UNDERLINE_ON + ";"s;
        }
        if (inverse) {
            res += Setting::INVERSE_ON + ";"s;
        }

        res += mp[Front][0] + ";"s;
        res += mp[Back][1] + "m"s;
        res += view;
        res += TAIL;
        return res;
    };

#ifdef DEBUG

    static void test() {
        std::string str{"Hello World !!!"};
        std::cout << AnsiColor::colorize<AnsiColor::YELLOW>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::RED>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::GREEN>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::BLUE>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::MAGENTA>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::CYAN>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::WHITE>(str) << "\n";

        std::cout << AnsiColor::colorize<AnsiColor::YELLOW, RED>(str, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::RED, GREEN>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::GREEN, MAGENTA>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::BLUE, CYAN>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::MAGENTA, WHITE>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::CYAN, BLUE>(str) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::WHITE, BLACK>(str) << "\n";

        std::cout << AnsiColor::colorize<AnsiColor::YELLOW, RED>(str, true, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::RED, GREEN>(str, false, true, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::GREEN, MAGENTA>(str, true, false, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::BLUE, CYAN>(str, true, true, false) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::MAGENTA, WHITE>(str, false, false, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::CYAN, BLUE>(str, true, false, true) << "\n";
        std::cout << AnsiColor::colorize<AnsiColor::WHITE, BLACK>(str, true, true, true) << "\n";
    }

#endif

private:

    struct Setting {
        inline static constexpr auto RESET = "0";

        inline static constexpr auto BRIGHT_ON = "1";
        inline static constexpr auto BRIGHT_OFF = "21";

        inline static constexpr auto UNDERLINE_ON = "4";
        inline static constexpr auto UNDERLINE_OFF = "24";

        inline static constexpr auto INVERSE_ON = "7";
        inline static constexpr auto INVERSE_OFF = "27";
    };

    struct ColorCode {
        inline static constexpr auto FG_BLACK = "30";
        inline static constexpr auto FG_RED = "31";
        inline static constexpr auto FG_GREEN = "32";
        inline static constexpr auto FG_YELLOW = "33";
        inline static constexpr auto FG_BLUE = "34";
        inline static constexpr auto FG_MAGENTA = "35";
        inline static constexpr auto FG_CYAN = "36";
        inline static constexpr auto FG_WHITE = "37";

        inline static constexpr auto BG_BLACK = "40";
        inline static constexpr auto BG_RED = "41";
        inline static constexpr auto BG_GREEN = "42";
        inline static constexpr auto BG_YELLOW = "43";
        inline static constexpr auto BG_BLUE = "44";
        inline static constexpr auto BG_MAGENTA = "45";
        inline static constexpr auto BG_CYAN = "46";
        inline static constexpr auto BG_WHITE = "47";
    };

    inline static const char *mp[Color::SIZE][2]{
            {ColorCode::FG_BLACK,   ColorCode::BG_BLACK},
            {ColorCode::FG_RED,     ColorCode::BG_RED},
            {ColorCode::FG_GREEN,   ColorCode::BG_GREEN},
            {ColorCode::FG_YELLOW,  ColorCode::BG_YELLOW},
            {ColorCode::FG_BLUE,    ColorCode::BG_BLUE},
            {ColorCode::FG_MAGENTA, ColorCode::BG_MAGENTA},
            {ColorCode::FG_CYAN,    ColorCode::BG_CYAN},
            {ColorCode::FG_WHITE,   ColorCode::BG_WHITE},
    };
};

#endif //ANSI_COLOR_ANSI_COLOR_H
