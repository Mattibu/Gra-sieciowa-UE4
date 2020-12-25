#pragma once

#include "CoreMinimal.h"

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
            spdlog::base_sink<Mutex>::formatter_->format(msg, formatted);
            std::string message = fmt::to_string(formatted);
            switch (msg.level)
            {
                case spdlog::level::trace:
                    UE_LOG(SpaceMMA, Verbose, TEXT(message.c_str()));
                    break;
                case spdlog::level::debug:
                    UE_LOG(SpaceMMA, Log, TEXT(message.c_str()));
                    break;
                case spdlog::level::info:
                    UE_LOG(SpaceMMA, Display, TEXT(message.c_str()));
                    break;
                case spdlog::level::warn:
                    UE_LOG(SpaceMMA, Warning, TEXT(message.c_str()));
                    break;
                case spdlog::level::err:
                    UE_LOG(SpaceMMA, Error, TEXT(message.c_str()));
                    break;
                case spdlog::level::critical:
                    UE_LOG(SpaceMMA, Fatal, TEXT(message.c_str()));
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