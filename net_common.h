#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <queue>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/error_code.hpp>


namespace io = asio;
using tcp = io::ip::tcp;
using error_code = asio::error_code;
using namespace std::placeholders;
using message_handler = std::function<void(std::string)>;
using error_handler = std::function<void()>;
