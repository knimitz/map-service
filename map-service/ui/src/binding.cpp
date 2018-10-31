/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdarg.h>
#include <sys/socket.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <errno.h>
#include "binding.hpp"

#define ELOG(args,...) _ELOG(__FUNCTION__,__LINE__,args,##__VA_ARGS__)
#ifdef DEBUGMODE
 #define DLOG(args,...) _DLOG(__FUNCTION__,__LINE__,args,##__VA_ARGS__)
#else
 #define DLOG(args,...)
#endif
static void _DLOG(const char* func, const int line, const char* log, ...);
static void _ELOG(const char* func, const int line, const char* log, ...);

using namespace std;

constexpr const char *const wmAPI = "windowmanager";
constexpr const char *const mpPrvAPI = "map-private";
static const char _new_req[] = "new_request";
static const char _sync_draw[] = "syncDraw";
static const char g_kKeyDrawingName[] = "drawing_name";
static const char g_kKeyDrawingArea[] = "drawing_area";
static const char g_kKeyDrawingRect[] = "drawing_rect";
static const char g_kKeySurface[] = "surface";
static const char g_kKeyUuid[] = "uuid";
static const char g_kKeyAppId[] = "appid";
static const char g_kKeyResponse[] = "response";
static const char g_verb_endDraw[] = "endDraw";

static void _on_hangup_static(void *closure, struct afb_wsj1 *wsj)
{
    static_cast<Binding*>(closure)->on_hangup(NULL,wsj);
}

static void _on_call_static(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
    /* Binding is not called from other process */
}

static void _on_event_static(void* closure, const char* event, struct afb_wsj1_msg *msg)
{
    static_cast<Binding*>(closure)->on_event(NULL,event,msg);
}

static void _on_reply_static(void *closure, struct afb_wsj1_msg *msg)
{
    static_cast<Binding*>(closure)->on_reply(NULL,msg);
}

static void *event_loop_run(void *args)
{
    struct sd_event* loop = (struct sd_event*)(args);
    DLOG("start eventloop");
    for(;;)
        sd_event_run(loop, 30000000);
}

Binding::Binding()
    : _wmh()
{
}

Binding::~Binding()
{
    if(mploop)
    {
        sd_event_unref(mploop);
    }
    if(this->wsj1 != NULL)
    {
        afb_wsj1_unref(this->wsj1);
    }
}


/**
 * This function is initialization function
 *
 * #### Parameters
 * - port  [in] : This argument should be specified to the port number to be used for websocket
 * - token [in] : This argument should be specified to the token to be used for websocket
 *
 * #### Rreturn
 * Returns 0 on success or -1 in case of error.
 *
 * #### Note
 *
 */
int Binding::init(int port, const string& token)
{
    int ret;
    if(port > 0 && token.size() > 0)
    {
        mport = port;
        mtoken = token;
    }
    else
    {
        ELOG("port and token should be > 0, Initial port and token uses.");
        return -1;
    }

    ret = initialize_websocket();
    if(ret != 0 )
    {
        ELOG("Failed to initialize websocket");
        return -1;
    }
    ret = run_eventloop();
    if(ret == -1){
        ELOG("Failed to create thread");
        return -1;
    }
    return 0;
}

int Binding::initialize_websocket()
{
    mploop = NULL;
    int ret = sd_event_default(&mploop);
    if(ret < 0)
    {
        ELOG("Failed to create event loop");
        goto END;
    }
    /* Initialize interface from websocket */
    {
    minterface.on_hangup = _on_hangup_static;
    minterface.on_call = _on_call_static;
    minterface.on_event = _on_event_static;
    string muri = "ws://localhost:" + to_string(mport) + "/api?token=" + mtoken;
    this->wsj1 = afb_ws_client_connect_wsj1(mploop, muri.c_str(), &minterface, this);
    }
    if(this->wsj1 == NULL)
    {
        ELOG("Failed to create websocket connection");
        goto END;
    }

    return 0;
END:
    if(mploop)
    {
        sd_event_unref(mploop);
    }
    return -1;
}

#define EVENT_SYNC_DRAW_NUM 5

void Binding::set_event_handler(const MyHandler& wmh)
{
    // Subscribe
    const char* ev = "event";

    if(wmh.on_sync_draw != nullptr) {
        struct json_object* j = json_object_new_object();
        json_object_object_add(j, ev, json_object_new_int(EVENT_SYNC_DRAW_NUM));

        int ret = afb_wsj1_call_j(this->wsj1, wmAPI, "wm_subscribe", j, _on_reply_static, this);
        if (0 > ret) {
            ELOG("Failed to subscribe event active");
        }
    }

    if(wmh.on_new_request != nullptr) {
        struct json_object* j = json_object_new_object();
        int ret = afb_wsj1_call_j(this->wsj1, mpPrvAPI, "subscribe", j, _on_reply_static, this);
        if (0 > ret) {
            ELOG("Failed to subscribe event active");
        }
    }

    // Register
    this->_wmh.on_reply = wmh.on_reply;
    this->_wmh.on_sync_draw = wmh.on_sync_draw;
    this->_wmh.on_new_request = wmh.on_new_request;
}

void Binding::end_draw(const char* role) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, g_kKeyDrawingName, json_object_new_string(role));
    this->call(wmAPI, g_verb_endDraw, object);
}

void Binding::provide_surface(const NewRequest& req) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, g_kKeyUuid, json_object_new_string(req.uuid.c_str()));
    json_object_object_add(object, g_kKeyAppId, json_object_new_string(req.appid.c_str()));
    this->call(wmAPI, g_verb_endDraw, object);
}

int Binding::run_eventloop()
{
    if(mploop && this->wsj1)
    {
        pthread_t thread_id;
        int ret = pthread_create(&thread_id, NULL, event_loop_run, mploop);
        if(ret != 0)
        {
            ELOG("Cannot run eventloop due to error:%d", errno);
            return -1;
        }
        else
            return 0;
    }
    else
    {
        ELOG("Connecting is not established yet");
        return -1;
    }
}

/**
 * This function calls the API of Audio Manager via WebSocket
 *
 * #### Parameters
 * - verb [in] : This argument should be specified to the API name (e.g. "connect")
 * - arg  [in] : This argument should be specified to the argument of API. And this argument expects JSON object
 *
 * #### Rreturn
 * - Returns 0 on success or -1 in case of error.
 *
 * #### Note
 * To call Audio Manager's APIs, the application should set its function name, arguments to JSON format.
 *
 */
int Binding::call(const string& api, const string& verb, struct json_object* arg)
{
    int ret;
    if(!this->wsj1)
    {
        return -1;
    }
    ret = afb_wsj1_call_j(this->wsj1, api.c_str(), verb.c_str(), arg, _on_reply_static, this);
    if (ret < 0) {
        ELOG("Failed to call verb:%s",verb.c_str());
    }
    return ret;
}

/************* Callback Function *************/

void Binding::on_hangup(void *closure, struct afb_wsj1 *wsj)
{
    DLOG("%s called", __FUNCTION__);
    if(onHangup != nullptr)
    {
        onHangup();
    }
}

void Binding::on_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
}

void Binding::on_event(void *closure, const char *event, struct afb_wsj1_msg *msg)
{
    /* check event is for us */
    string ev = string(event);
    if (ev.find(mpPrvAPI) == string::npos || ev.find(wmAPI) == string::npos) {
        /* It's not us */
        return;
    }
    struct json_object* object = afb_wsj1_msg_object_j(msg);
    if(ev.find(_new_req)) {
        json_object *j_val;
        json_object_object_get_ex(object, g_kKeySurface, &j_val);
        int surface_id = json_object_get_string(j_val);
        json_object_object_get_ex(object, g_kKeyAppId, &j_val);
        const char* appid  = json_object_get_string(j_val);
        json_object_object_get_ex(object, g_kKeyUuid, &j_val);
        const char* uuid = json_object_get_string(j_val);
        NewRequest nw_req = {appid, uuid, surface_id};
        this->_wmh.on_new_request(nw_req);
    }
    else if(ev.find(_sync_draw)) {
        json_object *j_val, *j_rect;
        json_object_object_get_ex(object, g_kKeyDrawingName, &j_val);
        const char* role = json_object_get_string(j_val);
        json_object_object_get_ex(object, g_kKeyDrawingArea, &j_val);
        const char* area = json_object_get_string(j_val);
        json_object_object_get_ex(object, g_kKeyDrawingRect, &j_rect);
        json_object_object_get_ex(j_rect, "x", &j_val);
        int x = json_object_get_int(j_val);
        json_object_object_get_ex(j_rect, "y", &j_val);
        int y = json_object_get_int(j_val);
        json_object_object_get_ex(j_rect, "width", &j_val);
        int w = json_object_get_int(j_val);
        json_object_object_get_ex(j_rect, "height", &j_val);
        int h = json_object_get_int(j_val);
        Rect rect(x, y, w, h);
        this->_wmh.on_sync_draw(role, area, rect);
    }

}

void Binding::on_reply(void *closure, struct afb_wsj1_msg *msg)
{
    struct json_object* reply = afb_wsj1_msg_object_j(msg);
    if(this->_wmh.on_reply)
    {
        this->_wmh.on_reply(reply);
    }
}

/* Internal Function in Binding */

static void _ELOG(const char* func, const int line, const char* log, ...)
{
    char *message;
    va_list args;
    va_start(args, log);
    if (log == NULL || vasprintf(&message, log, args) < 0)
        message = NULL;
    cout << "[ERROR: soundmanager]" << func << "(" << line << "):" << message << endl;
    va_end(args);
    free(message);
}

static void _DLOG(const char* func, const int line, const char* log, ...)
{
    char *message;
    va_list args;
    va_start(args, log);
    if (log == NULL || vasprintf(&message, log, args) < 0)
        message = NULL;
    cout << "[DEBUG: soundmanager]" << func << "(" << line << "):" << message << endl;
    va_end(args);
    free(message);
}
