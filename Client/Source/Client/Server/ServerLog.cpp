#include "ServerLog.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> spacemma::ServerLog::logger{};
const std::string spacemma::ServerLog::SERVER_LOG_NAME{"SpaceMMA"};
const std::string spacemma::ServerLog::SERVER_LOG_FILENAME{"spacemma.log"};
const size_t spacemma::ServerLog::SERVER_LOG_FILE_SIZE{8'388'608ULL};
const size_t spacemma::ServerLog::SERVER_LOG_MAX_FILES{3ULL};

spdlog::logger* spacemma::ServerLog::getLogger()
{
    if (!logger)
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto rotatingFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            SERVER_LOG_FILENAME, SERVER_LOG_FILE_SIZE, SERVER_LOG_MAX_FILES);
        spdlog::sinks_init_list sinkList{consoleSink, rotatingFileSink};
        logger = std::make_shared<spdlog::logger>(SERVER_LOG_NAME, sinkList);
        logger->set_level(static_cast<spdlog::level::level_enum>(SERVER_LOG_LEVEL));
        set_default_logger(logger);
    }
    return logger.get();
}
