#pragma once

#include <string>
#include <variant>

namespace cuebin {

enum class ErrorCode {
    // Parse errors
    InvalidCueFormat,
    InvalidMSF,
    InvalidTrackMode,
    InvalidFileType,
    MissingFile,
    MissingTrack,
    DuplicateIndex,
    UnexpectedDirective,

    // I/O errors
    FileNotFound,
    FileReadError,
    FileSeekError,

    // Disc errors
    LBAOutOfRange,
    TrackNotFound,
    InvalidArgument,
};

struct Error {
    ErrorCode code;
    std::string message;
    const char* sourceFile;
    int sourceLine;

    Error(ErrorCode code, std::string message,
          const char* sourceFile = __builtin_FILE(),
          int sourceLine = __builtin_LINE())
        : code(code)
        , message(std::move(message))
        , sourceFile(sourceFile)
        , sourceLine(sourceLine)
    {}
};

template <typename T>
class Result {
public:
    Result(T value) : m_storage(std::move(value)) {}
    Result(Error error) : m_storage(std::move(error)) {}

    bool ok() const noexcept { return std::holds_alternative<T>(m_storage); }
    explicit operator bool() const noexcept { return ok(); }

    const T& value() const& { return std::get<T>(m_storage); }
    T& value() & { return std::get<T>(m_storage); }
    T&& value() && { return std::get<T>(std::move(m_storage)); }

    const Error& error() const& { return std::get<Error>(m_storage); }
    Error& error() & { return std::get<Error>(m_storage); }

    const T* operator->() const { return &std::get<T>(m_storage); }
    T* operator->() { return &std::get<T>(m_storage); }

    const T& operator*() const& { return std::get<T>(m_storage); }
    T& operator*() & { return std::get<T>(m_storage); }
    T&& operator*() && { return std::get<T>(std::move(m_storage)); }

private:
    std::variant<T, Error> m_storage;
};

#define LIBCUEBIN_ERROR(code, msg) \
    ::cuebin::Error(code, msg, __FILE__, __LINE__)

#define LIBCUEBIN_TRY(expr)                         \
    ({                                               \
        auto&& _result = (expr);                     \
        if (!_result) return std::move(_result).error(); \
        std::move(*_result);                         \
    })

} // namespace cuebin
