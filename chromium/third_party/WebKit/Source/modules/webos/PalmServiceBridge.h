#ifndef PalmServiceBridge_h
#define PalmServiceBridge_h

#include "core/dom/ActiveDOMObject.h"
#include "core/dom/StringCallback.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/V8Binding.h"
#include "core/events/Event.h"
#include "core/events/EventListener.h"
#include "core/events/EventTarget.h"
#include "LunaServiceMgr.h"
#include <wtf/OwnPtr.h>

// #include <heap/Strong.h>
// #include <heap/StrongInlines.h>

#include <glib.h>
#include <list>


namespace blink {

class Document;


class PalmServiceBridge : public RefCounted<PalmServiceBridge>,
                          public LunaServiceManagerListener,
                          public ActiveDOMObject, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
    public:
        static PassRefPtr<PalmServiceBridge> create(ExecutionContext* context, bool subscribe = false)
        {
            return adoptRef(new PalmServiceBridge(context, subscribe));
        }

        bool init(Document*, bool subscribed = false);
        ~PalmServiceBridge();

        static int numHandlesForUrl(const char* appId);
        static void handlesForUrl(const char* appId, std::list<PalmServiceBridge*>& outHandles);

        virtual PalmServiceBridge* toPalmServiceBridge() { return this; }

        static void detachServices(Document*);
        static void cancelServices(Document*);

        String version();

        int token();

        int call(const String& uri, const String& payload, ExceptionState&);
        void cancel();
        virtual void onservicecallback(const String&);

        // callback from LunaServiceManagerListener
        virtual void serviceResponse(const char* body);

        Document* document() const;

        // ActiveDOMObject:
        virtual void contextDestroyed();
        virtual bool canSuspend() const;
        virtual void stop();

    private:
        bool m_canceled;
        bool m_subscribed;
        bool m_inServiceCallback;
        char *m_identifier;
        bool m_isPrivileged;

        PalmServiceBridge(ExecutionContext*, bool);
        PalmServiceBridge();
};
}

#endif
