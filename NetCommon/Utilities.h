#pragma once

#include <memory>
#include <thread>
#include <optional>
#include <mutex>
#include <deque>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstdint>

#ifdef _WIN64
#define _WIN64_WINNT 0x0601
#endif

#define ASIO_STANDALONE
#include <asio.hpp> // Core of ASIO library
#include <asio\ts\buffer.hpp> // Part of ASIO which handles memory movement
#include <asio\ts\internet.hpp> // Part of ASIO which handles network communication