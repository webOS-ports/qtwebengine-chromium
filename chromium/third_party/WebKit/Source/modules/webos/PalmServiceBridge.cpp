#include "PalmServiceBridge.h"
#include "Logging.h"

#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/Frame.h"
#include "core/page/Page.h"
#include "core/frame/Settings.h"

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/ExceptionState.h"

#include <map>
#include <set>

bool LoggingInitialized = false;
PmLogContext LogContext;

namespace blink {

typedef std::set<PalmServiceBridge*> ServicesSet;
typedef std::map<Document*, ServicesSet*> ServicesSetMap;

static ServicesSetMap* servicesByDocument()
{
    static ServicesSetMap map;
    return &map;
}

int PalmServiceBridge::numHandlesForUrl(const char* appId)
{
    for (ServicesSetMap::iterator setIt = servicesByDocument()->begin(); setIt != servicesByDocument()->end(); ++setIt) {
        if (!strcmp(appId, setIt->first->Url().GetString().Utf8().data()))
            return setIt->second->size();
    }

    return 0;
}

void PalmServiceBridge::handlesForUrl(const char* appId, std::list<PalmServiceBridge*>& outHandles)
{
    outHandles.clear();
    for (ServicesSetMap::iterator setIt = servicesByDocument()->begin(); setIt != servicesByDocument()->end(); ++setIt) {
        if (!strcmp(appId, setIt->first->Url().GetString().Utf8().data())) {
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
    : ContextClient(context),
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

    Frame *frame = document()->GetFrame();
    Settings* settings = document()->GetSettings();
    if (settings != 0 && document()->GetPage()->MainFrame() == frame) {
        m_identifier = strdup(settings->GetLuneosAppIdentifier().Utf8().data());
    }
    else {
        v8::Local<v8::Value> identifier;

        identifier = document()->GetFrame()->GetScriptController().ExecuteScriptInMainWorldAndReturnValue(ScriptSourceCode("PalmSystem && PalmSystem.getIdentifierForFrame(window.frameElement.id, window.frameElement.src)"));

        // Failure is reported as a null string.
        if (identifier.IsEmpty() || !identifier->IsString())
            m_identifier = strdup("dummy_identifier 0");
        else
            m_identifier = strdup(ToCoreString(v8::Handle<v8::String>::Cast(identifier)).Utf8().data());
    }

    if (settings != 0)
        m_isPrivileged = settings->GetLuneosPriviledged();

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

    cancel();

    if (m_scriptState)
        m_scriptState->Clear();

    if (GetExecutionContext() && document())
        removeFromServicesByDocument(document(), this);

    if (m_identifier)
        free(m_identifier);
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
            (*sit)->cancel();
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
            br->cancel();
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

int PalmServiceBridge::call(const String& uri, const String& payload, ExceptionState& exceptionState)
{
    DEBUG("PalmServiceBridge[%p]: calling on uri %s payload %s (identifier %s privileged %d subscribed %d)",
          this, uri.Utf8().data(), payload.Utf8().data(), m_identifier, m_isPrivileged, m_subscribed);

    LunaServiceManager::instance()->call(uri.Utf8().data(), payload.Utf8().data(), this, m_identifier, m_isPrivileged);
    if (LSMESSAGE_TOKEN_INVALID == listenerToken) {
        exceptionState.ThrowDOMException(kEncodingError, "The LS2 call returned an invalid token.");
        cancel();
    }

    return (int)listenerToken;
}

void PalmServiceBridge::serviceResponse(const char* body)
{
    if (m_canceled || !document() || !m_scriptState)
        return;

    if (!body)
        body = "";

    DEBUG("PalmServiceBridge[%p]: got service response %s (identifier %s privileged %d subscribed %d)",
          this, body, m_identifier, m_isPrivileged, m_subscribed);

    m_callbackFct->Invoke(this, body);

   // document()->updateStyleIfNeeded();
}

void PalmServiceBridge::cancel()
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

void PalmServiceBridge::contextDestroyed()
{
    cancel();
}

bool PalmServiceBridge::HasPendingActivity() const
{
    return m_canceled == false && GetExecutionContext();
}

Document* PalmServiceBridge::document() const
{
    if(GetExecutionContext()->IsDocument())
        return static_cast<Document*>(GetExecutionContext());
        
    return nullptr;
}

void PalmServiceBridge::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  ContextClient::Trace(visitor);
}
} // namespace blink
