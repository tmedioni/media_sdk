#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT

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

    const std::regex path_prefix_regex(R"reg(^http(s)?:\/\/([^\/]+)((\/[^\/]+)*)\/([^\/]+)$)reg");
    using namespace std::string_literals;
    class Proxy
    {
    public:
        explicit Proxy(const std::string& url)
        {
            std::smatch url_matches;
            if (std::regex_match(url, url_matches,path_prefix_regex))
            {
                // First capture group is the s from https
                m_domain = url_matches[2];

                m_path = url_matches[3];
                m_path.push_back('/');

                m_resource = url_matches[4];
                spdlog::trace("Parsed input stream CDN domain: '{}' from '{}'", m_domain, url);
                spdlog::trace("Parsed input stream intermediate path: '{}' from '{}'", m_path, url);
                spdlog::trace("Parsed input stream resource: '{}' from '{}'", m_resource, url);
            }
            else
            {
                throw InvalidStreamUrlException();
            }
        }

        void requestHandler(const httplib::Request &request, httplib::Response &result) const
        {
            spdlog::trace("Request path: '{}'", request.path);
            spdlog::trace("Request body: '{}'", request.body);

            httplib::SSLClient client(m_domain.c_str());
            if (auto subrequest =  client.Get(m_resource.c_str()))
            {
                if (subrequest->status == 200)
                {
                    spdlog::trace("Subrequest content: {}", subrequest->body);
                }
                else
                {
                    spdlog::error("Subrequest to CDN error: {}", subrequest.error());
                }

            }
            else
            {
                spdlog::error("Subrequest to CDN error: {}", subrequest.error());
            }


            result.set_content("", "text/plain");
        }

    private:
        std::string m_domain;
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
