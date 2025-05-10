
#include <iostream>

#include "poller.h"

int main( int argc, char** argv ) {
    poller::Poller client;

    std::println( "request postman-echo.com" );
    client.performRequest(
        "https://postman-echo.com/get", []( poller::Result res ) {
            std::cout << "Req0 Code: " << res.code << std::endl;
            std::cout << "Req0 Data: '" << res.data << "'" << std::endl
                      << std::endl;
        } );

    std::println( "request www.gstatic.com" );
    client.performRequest(
        "http://www.gstatic.com/generate_204", [&]( poller::Result res1 ) {
            std::cout << "Req1 Code: " << res1.code << std::endl;
            std::cout << "Req1 Data: '" << res1.data << "'" << std::endl
                      << std::endl;
            client.performRequest(
                "http://httpbin.org/user-agent", []( poller::Result res2 ) {
                    std::cout << "Req1-2 Code: " << res2.code << std::endl;
                    std::cout << "Req1-2 Data: '" << res2.data << "'"
                              << std::endl
                              << std::endl;
                } );
        } );

    std::cin.get();

    return 0;
}
