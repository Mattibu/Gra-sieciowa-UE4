#pragma once

#include "Logging/LogMacros.h"
#include "Windows/MinWindows.h"

#include <spdlog/sinks/base_sink.h>

namespace spacemma
{
    template<typename Mutex>
    class UEConsoleSink : public spdlog::sinks::base_sink<Mutex>
    {
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
            std::string message = fmt::to_string(formatted);
            FString ueMsg{ message.c_str() };
            switch (msg.level)
            {
                case spdlog::level::trace:
                    UE_LOG(LogTemp, Verbose, TEXT("%s"), *ueMsg);
                    break;
                case spdlog::level::debug:
                    UE_LOG(LogTemp, Log, TEXT("%s"), *ueMsg);
                    break;
                case spdlog::level::info:
                    UE_LOG(LogTemp, Display, TEXT("%s"), *ueMsg);
                    break;
                case spdlog::level::warn:
                    UE_LOG(LogTemp, Warning, TEXT("%s"), *ueMsg);
                    break;
                case spdlog::level::err:
                    UE_LOG(LogTemp, Error, TEXT("%s"), *ueMsg);
                    break;
                case spdlog::level::critical:
                    UE_LOG(LogTemp, Fatal, TEXT("%s"), *ueMsg);
                    break;
                default:
                    break;
            }
        }

        void flush_() override
        {

        }
    };
    using UEConsoleSink_mt = UEConsoleSink<std::mutex>;
}
