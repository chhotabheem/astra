#include "UriShortenerApp.h"
#include "Http2Server.h"
#include "Http2Client.h"
#include "IServiceResolver.h"
#include <Provider.h>
#include <Log.h>
#include <Metrics.h>
#include <Http2Response.h>
#include <IRequest.h>
#include <IResponse.h>

namespace uri_shortener {

UriShortenerApp::UriShortenerApp(
    std::shared_ptr<domain::ILinkRepository> repo,
    std::shared_ptr<domain::ICodeGenerator> gen,
    std::shared_ptr<application::ShortenLink> shorten,
    std::shared_ptr<application::ResolveLink> resolve,
    std::shared_ptr<application::DeleteLink> del,
    std::unique_ptr<astra::http2::Http2Client> http2_client,
    std::unique_ptr<astra::service_discovery::IServiceResolver> resolver,
    std::shared_ptr<service::IDataServiceAdapter> data_adapter,
    std::unique_ptr<UriShortenerMessageHandler> msg_handler,
    std::unique_ptr<ObservableMessageHandler> obs_msg_handler,
    std::unique_ptr<astra::execution::StickyQueue> pool,
    std::unique_ptr<UriShortenerRequestHandler> req_handler,
    std::unique_ptr<ObservableRequestHandler> obs_req_handler,
    std::unique_ptr<astra::http2::Http2Server> server,
    std::unique_ptr<astra::resilience::AtomicLoadShedder> load_shedder
)
    : m_repo(std::move(repo))
    , m_gen(std::move(gen))
    , m_shorten(std::move(shorten))
    , m_resolve(std::move(resolve))
    , m_delete(std::move(del))
    , m_http2_client(std::move(http2_client))
    , m_resolver(std::move(resolver))
    , m_data_adapter(std::move(data_adapter))
    , m_msg_handler(std::move(msg_handler))
    , m_obs_msg_handler(std::move(obs_msg_handler))
    , m_pool(std::move(pool))
    , m_req_handler(std::move(req_handler))
    , m_obs_req_handler(std::move(obs_req_handler))
    , m_server(std::move(server))
    , m_load_shedder(std::move(load_shedder)) {}

UriShortenerApp::~UriShortenerApp() {
    if (m_pool) {
        m_pool->stop();
    }
}

UriShortenerApp::UriShortenerApp(UriShortenerApp&&) noexcept = default;
UriShortenerApp& UriShortenerApp::operator=(UriShortenerApp&&) noexcept = default;

int UriShortenerApp::run() {
    auto accepted = obs::counter("load_shedder.accepted");
    auto rejected = obs::counter("load_shedder.rejected");
    
    auto resilient = [this, accepted, rejected](std::shared_ptr<astra::router::IRequest> req, 
                                                  std::shared_ptr<astra::router::IResponse> res) {
        auto guard = m_load_shedder->try_acquire();
        if (!guard) {
            rejected.inc();
            obs::warn("Load shedder rejected request", {
                {"current", std::to_string(m_load_shedder->current_count())},
                {"max", std::to_string(m_load_shedder->max_concurrent())}
            });
            res->set_status(503);
            res->set_header("Content-Type", "application/json");
            res->set_header("Retry-After", "1");
            res->write(R"({"error": "Service overloaded"})");
            res->close();
            return;
        }
        
        accepted.inc();
        
        auto http_res = std::dynamic_pointer_cast<astra::http2::Http2Response>(res);
        if (http_res) {
            http_res->add_scoped_resource(
                std::make_unique<astra::resilience::LoadShedderGuard>(std::move(*guard)));
        }
        
        m_obs_req_handler->handle(req, res);
    };
    
    m_server->router().post("/shorten", resilient);
    m_server->router().get("/:code", resilient);
    m_server->router().del("/:code", resilient);
    
    m_server->router().get("/health", [](std::shared_ptr<astra::router::IRequest>, 
                                          std::shared_ptr<astra::router::IResponse> res) {
        res->set_status(200);
        res->set_header("Content-Type", "application/json");
        res->write(R"({"status": "ok"})");
        res->close();
    });

    obs::info("URI Shortener listening");
    obs::info("Using message-based architecture", {{"workers", std::to_string(m_pool->worker_count())}});
    obs::info("Load shedder enabled", {{"max_concurrent", std::to_string(m_load_shedder->max_concurrent())}});
    
    auto start_result = m_server->start();
    if (!start_result) {
        obs::error("Failed to start server");
        return 1;
    }
    
    m_server->join();
    return 0;
}

} // namespace uri_shortener

