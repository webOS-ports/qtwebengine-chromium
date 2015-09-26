#ifndef PalmServiceBridge_h
#define PalmServiceBridge_h

#include "bindings/core/v8/ActiveScriptWrappable.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/modules/v8/v8_service_callback.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/ScriptState.h"
#include "LunaServiceMgr.h"

#include <glib.h>
#include <list>


namespace blink {

class Document;

class PalmServiceBridge : public ContextClient, 
                          public LunaServiceManagerListener,
                          public ScriptWrappable,
                          public ActiveScriptWrappable<PalmServiceBridge> {
    DEFINE_WRAPPERTYPEINFO();
    USING_GARBAGE_COLLECTED_MIXIN(PalmServiceBridge)
    WTF_MAKE_NONCOPYABLE(PalmServiceBridge);
    public:
        static PalmServiceBridge *Create(ExecutionContext* context, bool subscribe = false)
        {
            return new PalmServiceBridge(context, subscribe);
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

        void setOnservicecallback(ScriptState *&cbState, V8ServiceCallback* cppValue) {
            m_callbackFct.reset(cppValue);
            m_scriptState.reset(new ScriptStateProtectingContext(cbState));
        }
        ScriptWrappable *onservicecallback(ScriptState *cbState) const { return nullptr; }

        // callback from LunaServiceManagerListener
        virtual void serviceResponse(const char* body);

        Document* document() const;

        // ContextLifecycleObserver:
        virtual void contextDestroyed();

        // ScriptWrappable.
        bool HasPendingActivity() const final;

        virtual void Trace(blink::Visitor*);
        
    private:
        std::unique_ptr<V8ServiceCallback> m_callbackFct;
        std::unique_ptr<ScriptStateProtectingContext> m_scriptState;

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
