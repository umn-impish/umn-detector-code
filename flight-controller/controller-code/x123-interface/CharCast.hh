#pragma once

inline constexpr char operator "" _ch(unsigned long long arg) noexcept
{ return static_cast<char>(arg); }
