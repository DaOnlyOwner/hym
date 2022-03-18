#pragma once

#include "fmt/color.h"
#include <stdexcept>

#define HYM_ERROR(str, ...) { fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::dark_red), "[Error] ");\
auto formatted = fmt::format(str,__VA_ARGS__);\
fmt::print("{}\n", formatted);\
throw std::runtime_error(formatted);\
}

#define HYM_NON_FATAL_ERROR(str, ...) { fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "[Recoverable Error] ");\
auto formatted = fmt::format(str,__VA_ARGS__);\
fmt::print("{}\n", formatted);\
}

#define HYM_WARN(str, ...) { fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange), "[Warning] ");\
auto formatted = fmt::format(str,__VA_ARGS__);\
fmt::print("{}\n",formatted);}

#define HYM_INFO(str, ...) { fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::light_blue), "[Info] ");\
auto formatted = fmt::format(str,__VA_ARGS__);\
fmt::print("{}\n",formatted);}

