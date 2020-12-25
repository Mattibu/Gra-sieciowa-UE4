#include "SpaceLog.h"
#include "Client/Shared/UEConsoleSink.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> spacemma::SpaceLog::logger{};
const std::string spacemma::SpaceLog::SPACEMMA_LOG_NAME{ "SpaceMMA" };
const std::string spacemma::SpaceLog::SPACEMMA_LOG_FILENAME{ "spacemma.log" };
const size_t spacemma::SpaceLog::SPACEMMA_LOG_FILE_SIZE{ 8'388'608ULL };
const size_t spacemma::SpaceLog::SPACEMMA_LOG_MAX_FILES{ 3ULL };

spdlog::logger* spacemma::SpaceLog::getLogger()
{
    if (!logger)
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto rotatingFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            SPACEMMA_LOG_FILENAME, SPACEMMA_LOG_FILE_SIZE, SPACEMMA_LOG_MAX_FILES);
        auto ueSink = std::make_shared<UEConsoleSink_mt>();
        ueSink->set_pattern("[%Y-%m-%d %T.%e] [%s:%#] %v");
        spdlog::sinks_init_list sinkList{ ueSink, rotatingFileSink };
        logger = std::make_shared<spdlog::logger>(SPACEMMA_LOG_NAME, sinkList);
        logger->set_level(static_cast<spdlog::level::level_enum>(SPACEMMA_LOG_LEVEL));
        set_default_logger(logger);
    }
    return logger.get();
}
