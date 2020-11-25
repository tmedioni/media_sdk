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

    const std::regex path_prefix_regex(R"reg(^http(s)?:\/\/[^\/]+\/)reg");

    class Proxy
    {
    public:
        explicit Proxy(const std::string& url)
        {
            std::smatch url_prefix;
            if (std::regex_search(url, url_prefix,path_prefix_regex))
            {
                if (url_prefix.suffix().length() == 0)
                    throw InvalidStreamUrlException();

                m_path = url_prefix[0];
                m_resource = url_prefix.suffix();
                spdlog::trace("Parsed input stream address: '{}' from '{}'", m_path, url);
                spdlog::trace("Parsed input stream resource: '{}' from '{}'", m_resource, url);
            }
            else
            {
                throw InvalidStreamUrlException();
            }
        }

        void requestHandler(const httplib::Request &req, httplib::Response &res)
        {
            spdlog::trace("Request path: '{}'", req.path);
            spdlog::trace("Request body: '{}'", req.body);
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
    spdlog::set_level(spdlog::level::trace);
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
