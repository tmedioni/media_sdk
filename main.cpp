#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>
#include <spdlog/spdlog.h>

namespace {


    struct InvalidStreamUrlException : public std::exception
    {
        const char * what() const noexcept override
        {
            return "Check the input stream Url. It has to point towards an m3u8 resource.";
        }
    };

    const std::regex path_prefix_regex(R"reg(^http(s)?:\/\/([^\/]+)$)reg");
    const std::regex absolute_entry_regex(R"reg(^(http(s)?:\/\/([^\/]+))(.*)$)reg");

using namespace std::string_literals;
    class Proxy
    {
    public:
        explicit Proxy(const std::string& url)
        {
            std::smatch url_matches;
            if (std::regex_match(url, url_matches,path_prefix_regex))
            {
                m_domain = url;

                spdlog::info("Parsed input stream CDN domain: '{}' from '{}'", m_domain, url);
            }
            else
            {
                throw InvalidStreamUrlException();
            }
        }

        void requestHandler(const httplib::Request &request, httplib::Response &result)
        {
            spdlog::trace("Request path: '{}'", request.path);
            spdlog::trace("Request body: '{}'", request.body);

            const std::string url = request.path;
            const std::string domain = getServerUrl(url);

            spdlog::trace("Target CDN domain: '{}'", domain);

            httplib::Client client(domain.c_str());

            const bool manifest = isManifest(url);

            if (manifest && m_is_playing)
            {
                spdlog::info("[TRACK SWITCH]");
            }
            m_is_playing = !manifest;

            const char * type = isManifest(url) ? "MANIFEST" : "SEGMENT";
            spdlog::info("[IN][{}] http://localhost:8080{}", type, url);
            auto start_time = std::chrono::system_clock::now();

            if (auto subrequest =  client.Get(url.c_str()))
            {
                if (subrequest->status == 200)
                {
                    spdlog::trace("Subrequest received: {}...", subrequest->body.substr(0, 20));

                    if (!manifest)
                    {
                        result.set_content(subrequest->body, "audio/x-mpegurl");
                    }
                    else
                    {
                        result.set_content(processManifest(subrequest->body), "audio/x-mpegurl");
                    }
                    auto end_time = std::chrono::system_clock::now();
                    spdlog::info("[OUT][{}] http://localhost:8080{} ({}ms)", type, url, std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
                    return;
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
        std::atomic_bool m_is_playing = false;

        std::mutex m_absolute_links_mut;
        std::unordered_map<std::string, std::string> m_absolute_links; // Resource path -> absolute url server

        static bool isManifest(const std::string& url)
        {
            return url.size() > 5 && url.substr(url.size() - 5) == ".m3u8"s;
        }

        std::string processManifest(const std::string& body)
        {
            std::string result;

            std::istringstream iss(body);

            for (std::string line; std::getline(iss, line); )
            {
                std::smatch absolute_match;
                if (std::regex_match(line, absolute_match, absolute_entry_regex))
                {
                    const std::string resource = absolute_match[4].str();
                    const std::string domain = absolute_match[1].str();
                    // Capture groups 2 and 3 are the (s) of http(s) and the repeated non / character.
                    spdlog::trace("Found absolute link, address '{}', resource '{}'", domain, resource);

                    {
                        std::lock_guard<std::mutex> lock(m_absolute_links_mut);
                        m_absolute_links.emplace(resource, domain);
                    }

                    result += resource;
                }
                else
                {
                    result += line;
                }
                result+= "\n";
            }
            return result;
        }

        std::string getServerUrl(const std::string& resource_path)
        {
            std::lock_guard<std::mutex> lock(m_absolute_links_mut);

            if (auto found = m_absolute_links.find(resource_path); found != m_absolute_links.end())
            {
                spdlog::trace("Found resource '{}' bound to an absolute url '{}'", found->first, found->second);
                return found->second;
            }
            return m_domain;
        }
    };
}

using namespace std::placeholders;
int main(int argc, char **argv)
{
    //spdlog::set_level(spdlog::level::trace);
    if (argc < 2)
    {
        spdlog::error("Usage: ./media_sdk http://stream-url.io");
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
