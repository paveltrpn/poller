
import coindesk;

auto main( int argc, char** argv ) -> int {
    auto coindesk = coindesk::CoindeskClient{};

    coindesk.run();

    return 0;
}