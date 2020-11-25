#include <iostream>

#include <httplib.h>
#include <spdlog/spdlog.h>

namespace {


    struct InvalidStreamUrlException : public std::exception
    {
        const char * what() const noexcept override
        {
            return "Check the input stream Url. It has to point towards an m2u8 resource.";
        }
    };

    const std::regex path_prefix(R"reg(^http(s)?:\/\/[^\/]+\/)reg");

    class Proxy
    {
    public:
        explicit Proxy(std::string url)
        {
            if (!std::regex_search(url, path_prefix)) {
                throw InvalidStreamUrlException();
            }
        }

        void requestHandler(const httplib::Request &req, httplib::Response &res)
        {
            std::cout << "[IN]" << req.path << std::endl;
            std::cout << "Body: '" << req.body << "'" << std::endl;
            res.set_content("", "text/plain");
        }

    private:
        std::string m_path;
        std::string m_resource;
    };

}

using namespace std::placeholders;
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        spdlog::error("Usage: ./media_sdk http://stream-url/path/stream.m3u8");
        return -1;
    }

    try
    {
        Proxy proxy(argv[1]);

        httplib::Server server;

        server.Get(".*", [&](const auto& rq, auto& res) { return proxy.requestHandler(rq, res); });
        server.listen("0.0.0.0", 8080);
    }
    catch (const InvalidStreamUrlException& e)
    {
        spdlog::error("Error: {}", e.what());
        return -2;
    }
    catch (...)
    {
        spdlog::error("Something wrong happened...");
        return -3;
    }

    return 0;
}
