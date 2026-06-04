
module;

#include <sstream>
#include <iostream>
#include <format>
#include <source_location>
#include <exception>

export module log:log;

namespace tire::log {

/*
 * Example - "\033[1;32m[info] \033[0m"
 * here, \033 is the ESC character, ASCII 27. It is followed by [, then zero or more numbers separated by ;, and
 * finally the letter m. The numbers describe the colour and format to switch to from that point onwards.
 *
 * The codes for foreground and background colours are:
 *
 *          foreground background
 * black        30         40
 * red          31         41
 * green        32         42
 * yellow       33         43
 * blue         34         44
 * magenta      35         45
 * cyan         36         46
 * white        37         47
 *
 * Additionally, you can use these:
 *
 * reset             0  (everything back to normal)
 * bold/bright       1  (often a brighter shade of the same colour)
 * italic            3
 * underline         4
 * blink             5
 * inverse           7  (swap foreground and background colours)
 * crossed           9
 * bold/bright off  21
 * underline off    24
 * inverse off      27
 **/

auto elideRight( const char *str, int after ) -> std::string {
    const auto length = std::strlen( str );
    auto elided = std::stringstream{};

    if ( length > after ) {
        elided << std::format( "{}...", std::string_view( str ).substr( 0, length - after ) );
    } else {
        elided << str;
    }

    return elided.str();
}

auto elideLeft( const char *str, int after ) -> std::string {
    const auto length = std::strlen( str );
    auto elided = std::stringstream{};

    if ( length > after ) {
        elided << std::format( "...{}", std::string_view( str ).substr( length - after, length ) );
    } else {
        elided << str;
    }

    return elided.str();
}

export {
    //
#define ELIDE_AFTER 18
#define LOG_FILE std::cout

#define ENABLE_INFO_OUTPUT true
    template <bool enable = ENABLE_INFO_OUTPUT>
    auto info( const std::source_location &location = std::source_location::current() ) {
        if constexpr ( enable ) {
            return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
                constexpr char preamble[] = "\033[3;32m[info] \033[0m";

                const auto elided = elideLeft( location.file_name(), ELIDE_AFTER );

                const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", elided, location.line() );

                const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

                LOG_FILE << std::format( "{}\t\t{} {}\n", preamble, sourceInfo, message );
            };
        }
    }

#define ENABLE_NOTICE_OUTPUT true
    template <bool enable = ENABLE_NOTICE_OUTPUT>
    auto notice( const std::source_location &location = std::source_location::current() ) {
        if constexpr ( enable || ENABLE_NOTICE_OUTPUT ) {
            return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
                constexpr char preamble[] = "\033[3;36m[notice] \033[0m";

                const auto elided = elideLeft( location.file_name(), ELIDE_AFTER );

                const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", elided, location.line() );

                const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

                LOG_FILE << std::format( "{}\t{} {}\n", preamble, sourceInfo, message );
            };
        }
    }

#define ENABLE_DEBUG_OUTPUT true
    template <bool enable = ENABLE_DEBUG_OUTPUT>
    auto debug( const std::source_location &location = std::source_location::current() ) {
        if constexpr ( enable ) {
            return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
                constexpr char preamble[] = "\033[3;35m[debug] \033[0m";

                const auto elided = elideLeft( location.file_name(), ELIDE_AFTER );

                const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", elided, location.line() );

                const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

                LOG_FILE << std::format( "{}\t{} {}\n", preamble, sourceInfo, message );
            };
        }
    }

#define ENABLE_WARNING_OUTPUT true
    template <bool enable = ENABLE_WARNING_OUTPUT>
    auto warning( const std::source_location &location = std::source_location::current() ) {
        if constexpr ( enable ) {
            return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
                constexpr char preamble[] = "\033[3;33m[warning] \033[0m";

                const auto elided = elideLeft( location.file_name(), ELIDE_AFTER );

                const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", elided, location.line() );

                const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

                LOG_FILE << std::format( "{}\t{} {}\n", preamble, sourceInfo, message );
            };
        }
    }

#define ENABLE_ERROR_OUTPUT true
    template <bool enable = ENABLE_ERROR_OUTPUT>
    auto error( const std::source_location &location = std::source_location::current() ) {
        if constexpr ( enable ) {
            return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
                constexpr char preamble[] = "\033[3;31m[error] \033[0m\t";
                const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

                std::cout << std::format( "{}\t{}\n", preamble, message );

                const auto source = elideLeft( location.file_name(), ELIDE_AFTER );
                const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", source, location.line() );

                std::cout << std::format( "\t\tfile: {}\n", sourceInfo );

                const auto function = location.function_name();
                const auto functionInfo = std::format( "\033[3;90m[{}] \033[0m", function );

                LOG_FILE << std::format( "\t\tfunc: {}\n", functionInfo );
            };
        }
    }

    auto fatal( const std::source_location &location = std::source_location::current() ) {
        return [&, location]<typename... Ts>( std::format_string<Ts...> msg = "", Ts &&...args ) -> void {
            constexpr char preamble[] = "\033[3;31m[fatal] \033[0m";
            const auto message = std::vformat( msg.get(), std::make_format_args( args... ) );

            std::cout << std::format( "{}\t{}\n", preamble, message );

            const auto source = elideLeft( location.file_name(), ELIDE_AFTER );
            const auto sourceInfo = std::format( "\033[3;90m[{}({})] \033[0m", source, location.line() );

            std::cout << std::format( "\t\tfile: {}\n", sourceInfo );

            const auto function = location.function_name();
            const auto functionInfo = std::format( "\033[3;90m[{}] \033[0m", function );

            LOG_FILE << std::format( "\t\tfunc: {}\n", functionInfo );

            // Terminate!!!
            std::terminate();
        };
    }

    template <bool enable = true, typename... Ts>
    void print_source( const std::source_location &location = std::source_location::current() ) {
        constexpr char preamble[] = "\033[3;34m[source] \033[0m";
        std::cout << preamble << "file: " << location.file_name() << '(' << location.line() << ':' << location.column()
                  << ") `" << location.function_name() << "`" << '\n';
    }
}

}  // namespace tire::log
