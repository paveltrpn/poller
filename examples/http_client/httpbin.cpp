
import httpbin;

auto main( int argc, char** argv ) -> int {
    auto httpbin = httpbin::HttpbinClient{};

    httpbin.run();

    return 0;
}
