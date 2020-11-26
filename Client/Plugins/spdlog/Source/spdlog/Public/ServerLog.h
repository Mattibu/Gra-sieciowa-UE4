#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <gsl/gsl-lite.hpp>
#include "spdlog/spdlog.h"

#define SERVER_LEVEL_TRACE 0
#define SERVER_LEVEL_DEBUG 1
#define SERVER_LEVEL_INFO 2
#define SERVER_LEVEL_WARN 3
#define SERVER_LEVEL_ERROR 4
#define SERVER_LEVEL_CRITICAL 5
#define SERVER_LEVEL_OFF 6

#ifndef SERVER_LOG_LEVEL
#define SERVER_LOG_LEVEL SERVER_LEVEL_DEBUG
#endif

#define SERVER_LOGGER_CALL(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_TRACE
#define SERVER_LOGGER_TRACE(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::trace, __VA_ARGS__)
#define SERVER_TRACE(...) SERVER_LOGGER_TRACE(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_TRACE(logger, ...) (void)0
#define SERVER_TRACE(...) (void)0
#endif

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_DEBUG
#define SERVER_LOGGER_DEBUG(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::debug, __VA_ARGS__)
#define SERVER_DEBUG(...) SERVER_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_DEBUG(logger, ...) (void)0
#define SERVER_DEBUG(...) (void)0
#endif

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_INFO
#define SERVER_LOGGER_INFO(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::info, __VA_ARGS__)
#define SERVER_INFO(...) SERVER_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_INFO(logger, ...) (void)0
#define SERVER_INFO(...) (void)0
#endif

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_WARN
#define SERVER_LOGGER_WARN(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::warn, __VA_ARGS__)
#define SERVER_WARN(...) SERVER_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_WARN(logger, ...) (void)0
#define SERVER_WARN(...) (void)0
#endif

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_ERROR
#define SERVER_LOGGER_ERROR(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::err, __VA_ARGS__)
#define SERVER_ERROR(...) SERVER_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_ERROR(logger, ...) (void)0
#define SERVER_ERROR(...) (void)0
#endif

#if SERVER_LOG_LEVEL <= SERVER_LEVEL_CRITICAL
#define SERVER_LOGGER_CRITICAL(logger, ...) SERVER_LOGGER_CALL(spacemma::ServerLog::getLogger(), spdlog::level::critical, __VA_ARGS__)
#define SERVER_CRITICAL(...) SERVER_LOGGER_CRITICAL(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SERVER_LOGGER_CRITICAL(logger, ...) (void)0
#define SERVER_CRITICAL(...) (void)0
#endif

namespace spacemma
{
    class ServerLog final
    {
    public:
        ServerLog() = delete;
        static spdlog::logger* getLogger();
    private:
        static std::shared_ptr<spdlog::logger> logger;
        static const std::string SERVER_LOG_NAME;
        static const std::string SERVER_LOG_FILENAME;
        static const size_t SERVER_LOG_FILE_SIZE;
        static const size_t SERVER_LOG_MAX_FILES;
    };
}
