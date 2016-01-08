#include <mbgl/storage/online_file_source.hpp>
#include <mbgl/storage/http_context_base.hpp>
#include <mbgl/storage/network_status.hpp>

#include <mbgl/storage/response.hpp>
#include <mbgl/platform/log.hpp>

#include <mbgl/util/thread.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/util/exception.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/timer.hpp>

#include <algorithm>
#include <cassert>
#include <set>
#include <unordered_map>

namespace mbgl {

class RequestBase;

class OnlineFileRequest : public FileRequest {
public:
    OnlineFileRequest(OnlineFileSource& fileSource_)
        : fileSource(fileSource_) {
    }

    ~OnlineFileRequest() {
        fileSource.cancel(this);
    }

    OnlineFileSource& fileSource;
    std::unique_ptr<WorkRequest> workRequest;
};

class OnlineFileRequestImpl : public util::noncopyable {
public:
    using Callback = std::function<void (Response)>;

    OnlineFileRequestImpl(const Resource&, Callback, OnlineFileSource::Impl&);
    ~OnlineFileRequestImpl();

    void networkIsReachableAgain(OnlineFileSource::Impl&);

private:
    void scheduleCacheRequest(OnlineFileSource::Impl&);
    void scheduleRealRequest(OnlineFileSource::Impl&, bool forceImmediate = false);

    const Resource resource;
    std::unique_ptr<WorkRequest> cacheRequest;
    RequestBase* realRequest = nullptr;
    util::Timer realRequestTimer;
    Callback callback;

    // The current response data. Used to create conditional HTTP requests, and to check whether
    // new responses we got changed any data.
    std::shared_ptr<const Response> response;

    // Counts the number of subsequent failed requests. We're using this value for exponential
    // backoff when retrying requests.
    int failedRequests = 0;
};

class OnlineFileSource::Impl {
public:
    using Callback = std::function<void (Response)>;

    Impl(FileCache*);
    ~Impl();

    void networkIsReachableAgain();

    void add(Resource, FileRequest*, Callback);
    void cancel(FileRequest*);

private:
    friend OnlineFileRequestImpl;

    std::unordered_map<FileRequest*, std::unique_ptr<OnlineFileRequestImpl>> pending;
    FileCache* const cache;
    const std::unique_ptr<HTTPContextBase> httpContext;
    util::AsyncTask reachability;
};

OnlineFileSource::OnlineFileSource(FileCache* cache)
    : thread(std::make_unique<util::Thread<Impl>>(
          util::ThreadContext{ "OnlineFileSource", util::ThreadType::Unknown, util::ThreadPriority::Low },
          cache)) {
}

OnlineFileSource::~OnlineFileSource() = default;

std::unique_ptr<FileRequest> OnlineFileSource::request(const Resource& resource, Callback callback) {
    if (!callback) {
        throw util::MisuseException("FileSource callback can't be empty");
    }

    std::string url;

    switch (resource.kind) {
    case Resource::Kind::Style:
        url = mbgl::util::mapbox::normalizeStyleURL(resource.url, accessToken);
        break;

    case Resource::Kind::Source:
        url = util::mapbox::normalizeSourceURL(resource.url, accessToken);
        break;

    case Resource::Kind::Glyphs:
        url = util::mapbox::normalizeGlyphsURL(resource.url, accessToken);
        break;

    case Resource::Kind::SpriteImage:
    case Resource::Kind::SpriteJSON:
        url = util::mapbox::normalizeSpriteURL(resource.url, accessToken);
        break;

    default:
        url = resource.url;
    }

    Resource res { resource.kind, url };
    auto req = std::make_unique<OnlineFileRequest>(*this);
    req->workRequest = thread->invokeWithCallback(&Impl::add, callback, res, req.get());
    return std::move(req);
}

void OnlineFileSource::cancel(FileRequest* req) {
    thread->invoke(&Impl::cancel, req);
}

// ----- Impl -----

OnlineFileSource::Impl::Impl(FileCache* cache_)
    : cache(cache_),
      httpContext(HTTPContextBase::createContext()),
      reachability(std::bind(&Impl::networkIsReachableAgain, this)) {
    // Subscribe to network status changes, but make sure that this async handle doesn't keep the
    // loop alive; otherwise our app wouldn't terminate. After all, we only need status change
    // notifications when our app is still running.
    NetworkStatus::Subscribe(&reachability);
}

OnlineFileSource::Impl::~Impl() {
    NetworkStatus::Unsubscribe(&reachability);
}

void OnlineFileSource::Impl::networkIsReachableAgain() {
    for (auto& req : pending) {
        req.second->networkIsReachableAgain(*this);
    }
}

void OnlineFileSource::Impl::add(Resource resource, FileRequest* req, Callback callback) {
    pending[req] = std::make_unique<OnlineFileRequestImpl>(resource, callback, *this);
}

void OnlineFileSource::Impl::cancel(FileRequest* req) {
    pending.erase(req);
}

// ----- OnlineFileRequest -----

OnlineFileRequestImpl::OnlineFileRequestImpl(const Resource& resource_, Callback callback_, OnlineFileSource::Impl& impl)
    : resource(resource_),
      callback(callback_) {
    if (impl.cache) {
        scheduleCacheRequest(impl);
    } else {
        scheduleRealRequest(impl);
    }
}

OnlineFileRequestImpl::~OnlineFileRequestImpl() {
    if (realRequest) {
        realRequest->cancel();
        realRequest = nullptr;
    }
    // realRequestTimer and cacheRequest are automatically canceled upon destruction.
}

void OnlineFileRequestImpl::scheduleCacheRequest(OnlineFileSource::Impl& impl) {
    // Check the cache for existing data so that we can potentially
    // revalidate the information without having to redownload everything.
    cacheRequest = impl.cache->get(resource, [this, &impl](std::shared_ptr<Response> response_) {
        cacheRequest = nullptr;

        if (response_) {
            response_->stale = response_->isExpired();
            response = response_;
            callback(*response);
        }

        scheduleRealRequest(impl);
    });
}

void OnlineFileRequestImpl::scheduleRealRequest(OnlineFileSource::Impl& impl, bool forceImmediate) {
    if (realRequest) {
        // There's already a request in progress; don't start another one.
        return;
    }

    // If we don't have a fresh response, we'll retry immediately. Otherwise, the timeout will
    // depend on how many consecutive errors we've encountered, and on the expiration time.
    Seconds timeout = (!response || response->stale) ? Seconds::zero() : Seconds::max();

    if (response && response->error) {
        assert(failedRequests > 0);
        switch (response->error->reason) {
        case Response::Error::Reason::Server: {
            // Retry immediately, unless we have a certain number of attempts already
            const int graceRetries = 3;
            if (failedRequests <= graceRetries) {
                timeout = Seconds(1);
            } else {
                timeout = Seconds(1 << std::min(failedRequests - graceRetries, 31));
            }
        } break;
        case Response::Error::Reason::Connection: {
            // Exponential backoff
            timeout = Seconds(1 << std::min(failedRequests - 1, 31));
        } break;
        default:
            // Do not retry due to error.
            break;
        }
    }

    // Check to see if this response expires earlier than a potential error retry.
    if (response && response->expires > Seconds::zero()) {
        // Only update the timeout if we don't have one yet, and only if the new timeout is shorter
        // than the previous one.
        timeout = std::min(timeout, std::max(Seconds::zero(), response->expires - toSeconds(SystemClock::now())));
    }

    if (forceImmediate) {
        timeout = Seconds::zero();
    }

    if (timeout == Seconds::max()) {
        return;
    }

    realRequestTimer.start(timeout, Duration::zero(), [this, &impl] {
        assert(!realRequest);
        realRequest = impl.httpContext->createRequest(resource.url, [this, &impl](std::shared_ptr<const Response> response_) {
            realRequest = nullptr;

            // Only update the cache for successful or 404 responses.
            // In particular, we don't want to write a Canceled request, or one that failed due to
            // connection errors to the cache. Server errors are hopefully also temporary, so we're not
            // caching them either.
            if (impl.cache &&
                (!response_->error || (response_->error->reason == Response::Error::Reason::NotFound))) {
                // Store response in database. Make sure we only refresh the expires column if the data
                // didn't change.
                FileCache::Hint hint = FileCache::Hint::Full;
                if (response && response_->data == response->data) {
                    hint = FileCache::Hint::Refresh;
                }
                impl.cache->put(resource, response_, hint);
            }

            response = response_;

            if (response->error) {
                failedRequests++;
            } else {
                // Reset the number of subsequent failed requests after we got a successful one.
                failedRequests = 0;
            }

            callback(*response);
            scheduleRealRequest(impl);
        }, response);
    });
}

void OnlineFileRequestImpl::networkIsReachableAgain(OnlineFileSource::Impl& impl) {
    // We need all requests to fail at least once before we are going to start retrying
    // them, and we only immediately restart request that failed due to connection issues.
    if (response && response->error && response->error->reason == Response::Error::Reason::Connection) {
        scheduleRealRequest(impl, true);
    }
}

} // namespace mbgl
