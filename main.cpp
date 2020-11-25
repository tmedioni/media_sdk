#include <iostream>
#include <httplib.h>

class Proxy
{
public:
    explicit Proxy(std::string url) : m_url(std::move(url)) {}

    void requestHandler(const httplib::Request &req, httplib::Response &res)
    {
        std::cout << "[IN]" << req.path << std::endl;
        std::cout << "Body: '" << req.body << "'" << std::endl;
        res.set_content("", "text/plain");
    }

private:
    std::string m_url;
};

using namespace std::placeholders;
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./media_sdk http://stream-url" << std::endl;
        return -1;
    }
    Proxy proxy(argv[1]);

    httplib::Server server;

    server.Get(".*", [&](const auto& rq, auto& res) { return proxy.requestHandler(rq, res); });
    server.listen("0.0.0.0", 8080);

    return 0;
}
