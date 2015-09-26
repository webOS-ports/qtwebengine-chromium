#include "config.h"
#include "PalmServiceBridge.h"
#include "Logging.h"

#include "core/dom/Document.h"
#include "core/events/Event.h"
// #include "core/events/EventException.h"
#include "core/events/EventListener.h"
// #include "core/events/EventNames.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/Frame.h"
#include "platform/Logging.h"
#include "core/page/Page.h"
#include "core/frame/Settings.h"
#include <wtf/text/WTFString.h>
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8Binding.h"
#include "core/dom/StringCallback.h"
#include <wtf/RefCountedLeakCounter.h>

/*
#include "JSDOMWindow.h"
#include "JSEventListener.h"
#include "JSFunction.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "runtime_root.h"
#include <runtime/JSLock.h>
using namespace JSC;
*/

#include <map>
#include <set>

bool LoggingInitialized = false;
PmLogContext LogContext;

namespace blink {

typedef std::set<PalmServiceBridge*> ServicesSet;
typedef std::map<Document*, ServicesSet*> ServicesSetMap;

#ifndef NDEBUG
static WTF::RefCountedLeakCounter serviceBridgeCounter("PalmServiceBridge");
#endif

static ServicesSetMap* servicesByDocument()
{
    static ServicesSetMap map;
    return &map;
}

int PalmServiceBridge::numHandlesForUrl(const char* appId)
{
    for (ServicesSetMap::iterator setIt = servicesByDocument()->begin(); setIt != servicesByDocument()->end(); ++setIt) {
        if (!strcmp(appId, setIt->first->url().string().utf8().data()))
            return setIt->second->size();
    }

    return 0;
}

void PalmServiceBridge::handlesForUrl(const char* appId, std::list<PalmServiceBridge*>& outHandles)
{
    outHandles.clear();
    for (ServicesSetMap::iterator setIt = servicesByDocument()->begin(); setIt != servicesByDocument()->end(); ++setIt) {
        if (!strcmp(appId, setIt->first->url().string().utf8().data())) {
            ServicesSet* set = setIt->second;

            for (ServicesSet::iterator s = set->begin(); s != set->end(); ++s)
                outHandles.push_back(*s);

            return;
        }
    }
}

static void addToServicesByDocument(Document* doc, PalmServiceBridge* svc)
{
    if (!doc || !svc)
        return;

    ServicesSet* set = 0;
    ServicesSetMap::iterator it = servicesByDocument()->find(doc);
    if (it == servicesByDocument()->end()) {
        set = new ServicesSet();
        (*servicesByDocument())[doc] = set;
    } else
        set = it->second;

    set->insert(svc);
}

static void removeFromServicesByDocument(Document* doc, PalmServiceBridge* svc)
{
    if (!doc || !svc)
        return;

    ServicesSetMap::iterator it = servicesByDocument()->find(doc);
    if (it == servicesByDocument()->end())
        return;

    ServicesSet* set = it->second;
    if (!set)
        return;

    set->erase(svc);
    if (!set->size()) {
        // remove from the hash map
        delete set;
        servicesByDocument()->erase(it);
    }
}

PalmServiceBridge::PalmServiceBridge(ExecutionContext* context, bool subscribe)
    : ActiveDOMObject(context),
      m_canceled(false),
      m_subscribed(subscribe),
      m_inServiceCallback(false),
      m_identifier(0),
      m_isPrivileged(false)
{
    if (!LoggingInitialized) {
        PmLogGetContext("QtWebEngineProcess", &LogContext);
        LoggingInitialized = true;
    }

    addToServicesByDocument(document(), this);

#ifndef NDEBUG
    serviceBridgeCounter.increment();
#endif
    Frame *frame = document()->frame();
    Settings* settings = document()->settings();
    if (settings != 0 && document()->page()->mainFrame() == frame) {
        m_identifier = strdup(settings->luneosAppIdentifier().utf8().data());
    }
    else {
        v8::Local<v8::Value> identifier;

        identifier = document()->frame()->script().executeScriptInMainWorldAndReturnValue(ScriptSourceCode("PalmSystem && PalmSystem.getIdentifierForFrame(window.frameElement.id, window.frameElement.src)"));
        m_identifier = strdup(toCoreString(v8::Handle<v8::String>::Cast(identifier)).utf8().data());
    }

    if (settings != 0)
        m_isPrivileged = settings->luneosPriviledged();

    DEBUG("PalmServiceBridge[%p]: created (subscribe %d identifier %s privileged %d)",
          this, subscribe, m_identifier, m_isPrivileged);
}

bool PalmServiceBridge::init(Document* d, bool subscribe)
{
    m_subscribed = subscribe;

    DEBUG("PalmServiceBridge[%p]: initialized (subscribe %d)", this, subscribe);

    return true;
}

PalmServiceBridge::~PalmServiceBridge()
{
    DEBUG("PalmServiceBridge[%p]: destroying (identifier %s privileged %d subscribed %d)",
          this, m_identifier, m_isPrivileged, m_subscribed);

    ExceptionCode ec;
    cancel(ec);

    if (executionContext() && document())
        removeFromServicesByDocument(document(), this);

    if (m_identifier)
        free(m_identifier);

#ifndef NDEBUG
    serviceBridgeCounter.decrement();
#endif
}

void PalmServiceBridge::detachServices(Document* doc)
{
    ServicesSetMap::iterator it = servicesByDocument()->find(doc);
    if (it == servicesByDocument()->end())
        return;

    ServicesSet* services = it->second;
    servicesByDocument()->erase(it);

    if (services) {
        while (services->size()) {
            ServicesSet::iterator sit = services->begin();
            ExceptionCode ec;
            (*sit)->cancel(ec);
            services->erase(sit);
        }
        delete services;
    }

}

void PalmServiceBridge::cancelServices(Document* doc)
{
    ServicesSetMap::iterator it = servicesByDocument()->find(doc);
    if (it == servicesByDocument()->end())
        return;

    ServicesSet* services = it->second;

    if (services) {
        for (ServicesSet::iterator sit = services->begin(); sit != services->end(); ++sit) {
            PalmServiceBridge* br = *sit;
            ExceptionCode ec;
            br->cancel(ec);
        }
    }
}

String PalmServiceBridge::version()
{
    return String("1.1");
}

int PalmServiceBridge::token()
{
    return (int)listenerToken;
}

int PalmServiceBridge::call(const String& uri, const String& payload, ExceptionCode& ec)
{
    DEBUG("PalmServiceBridge[%p]: calling on uri %s payload %s (identifier %s privileged %d subscribed %d)",
          this, uri.utf8().data(), payload.utf8().data(), m_identifier, m_isPrivileged, m_subscribed);

    LunaServiceManager::instance()->call(uri.utf8().data(), payload.utf8().data(), this, m_identifier, m_isPrivileged);
    if (LSMESSAGE_TOKEN_INVALID == listenerToken) {
        ExceptionCode ec;
        cancel(ec);
    }

    return (int)listenerToken;
}

void PalmServiceBridge::serviceResponse(const char* body)
{
    if (m_canceled || !document())
        return;

    if (!body)
        body = "";

    DEBUG("PalmServiceBridge[%p]: got service response %s (identifier %s privileged %d subscribed %d)",
          this, body, m_identifier, m_isPrivileged, m_subscribed);

    m_callbackFunction->handleEvent(String::fromUTF8(body));

   // document()->updateStyleIfNeeded();
}

void PalmServiceBridge::cancel(ExceptionCode& ec)
{
    if (m_canceled)
        return;

    m_canceled = true;
    if (listenerToken) {
        DEBUG("PalmServiceBridge[%p]: canceling current call (identifier %s privileged %d subscribed %d)",
            this, m_identifier, m_isPrivileged, m_subscribed);

        LunaServiceManager::instance()->cancel(this);
    }
}

void PalmServiceBridge::stop()
{
    ExceptionCode ec;

    DEBUG("PalmServiceBridge[%p]: stopping ... (identifier %s privileged %d subscribed %d)",
        this, m_identifier, m_isPrivileged, m_subscribed);

    cancel(ec);
}

bool PalmServiceBridge::canSuspend() const
{
    return false;
}

void PalmServiceBridge::contextDestroyed()
{
    ActiveDOMObject::contextDestroyed();
}

Document* PalmServiceBridge::document() const
{
    ASSERT(executionContext()->isDocument());
    return static_cast<Document*>(executionContext());
}

} // namespace blink
