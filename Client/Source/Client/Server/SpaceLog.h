#pragma once

#include "Windows/MinWindows.h"
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>
#include <gsl/gsl-lite.hpp>
#include "spdlog/spdlog.h"

#define SPACEMMA_LEVEL_TRACE 0
#define SPACEMMA_LEVEL_DEBUG 1
#define SPACEMMA_LEVEL_INFO 2
#define SPACEMMA_LEVEL_WARN 3
#define SPACEMMA_LEVEL_ERROR 4
#define SPACEMMA_LEVEL_CRITICAL 5
#define SPACEMMA_LEVEL_OFF 6

//#define SPACEMMA_LOG_LEVEL SPACEMMA_LEVEL_INFO

#ifndef SPACEMMA_LOG_LEVEL
#define SPACEMMA_LOG_LEVEL SPACEMMA_LEVEL_DEBUG
#endif

#define SPACEMMA_LOGGER_CALL(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_TRACE
#define SPACEMMA_LOGGER_TRACE(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::trace, __VA_ARGS__)
#define SPACEMMA_TRACE(...) SPACEMMA_LOGGER_TRACE(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_TRACE(logger, ...) (void)0
#define SPACEMMA_TRACE(...) (void)0
#endif

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_DEBUG
#define SPACEMMA_LOGGER_DEBUG(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::debug, __VA_ARGS__)
#define SPACEMMA_DEBUG(...) SPACEMMA_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_DEBUG(logger, ...) (void)0
#define SPACEMMA_DEBUG(...) (void)0
#endif

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_INFO
#define SPACEMMA_LOGGER_INFO(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::info, __VA_ARGS__)
#define SPACEMMA_INFO(...) SPACEMMA_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_INFO(logger, ...) (void)0
#define SPACEMMA_INFO(...) (void)0
#endif

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_WARN
#define SPACEMMA_LOGGER_WARN(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::warn, __VA_ARGS__)
#define SPACEMMA_WARN(...) SPACEMMA_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_WARN(logger, ...) (void)0
#define SPACEMMA_WARN(...) (void)0
#endif

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_ERROR
#define SPACEMMA_LOGGER_ERROR(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::err, __VA_ARGS__)
#define SPACEMMA_ERROR(...) SPACEMMA_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_ERROR(logger, ...) (void)0
#define SPACEMMA_ERROR(...) (void)0
#endif

#if SPACEMMA_LOG_LEVEL <= SPACEMMA_LEVEL_CRITICAL
#define SPACEMMA_LOGGER_CRITICAL(logger, ...) SPACEMMA_LOGGER_CALL(spacemma::SpaceLog::getLogger(), spdlog::level::critical, __VA_ARGS__)
#define SPACEMMA_CRITICAL(...) SPACEMMA_LOGGER_CRITICAL(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define SPACEMMA_LOGGER_CRITICAL(logger, ...) (void)0
#define SPACEMMA_CRITICAL(...) (void)0
#endif

namespace spacemma
{
    class SpaceLog final
    {
    public:
        SpaceLog() = delete;
        static spdlog::logger* getLogger();
    private:
        static std::shared_ptr<spdlog::logger> logger;
        static const std::string SPACEMMA_LOG_NAME;
        static const std::string SPACEMMA_LOG_FILENAME;
        static const size_t SPACEMMA_LOG_FILE_SIZE;
        static const size_t SPACEMMA_LOG_MAX_FILES;
    };
}
