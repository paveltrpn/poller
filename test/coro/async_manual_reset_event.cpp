
#include <iostream>
#include <coroutine>

import coro;

poller::async_manual_reset_event event;
std::string value;

void producer() {
    value = "get_some_string_value";

    // Publish a value by setting the event.
    event.set();
}

// Can be called many times to create many tasks.
// All consumer tasks will wait until value has been published.
poller::task<void> consumer( std::string_view whoAmI ) {
    // Wait until value has been published by awaiting event.

    if ( !event.is_set() ) {
        std::cout << whoAmI << ", by now cunsumption is not allowed!\n";
    }

    co_await event;  // suspend point!!!
    std::cout << whoAmI << ", consume value: " << value << "\n";
}

int main( int argc, char** argv ) {
    std::cout << "try to consume...\n";
    consumer( "first" );
    consumer( "second" );
    consumer( "third" );

    std::cout << "produce value...\n";
    producer();

    return 0;
}
