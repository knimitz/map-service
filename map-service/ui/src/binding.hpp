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

#ifndef BINDING_H
#define BINDING_H
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <json-c/json.h>
#include <systemd/sd-event.h>
#define AFB_BINDING_VERSION 3
extern "C"
{
#include <afb/afb-binding.h>
#include <afb/afb-wsj1.h>
#include <afb/afb-ws-client.h>
}

class Rect {
  public:
    Rect() : _x(0), _y(0),_w(0), _h(0) {}
    Rect(unsigned x, unsigned y, unsigned w, unsigned h)
        : _x(x), _y(y),_w(w), _h(h) {}
    ~Rect() = default;
    unsigned left()   const { return _x;}
    unsigned top()    const { return _y;}
    unsigned width()  const { return _w;}
    unsigned height() const { return _h;}
    void set_left  (unsigned int x) { _x = x; }
    void set_top   (unsigned int y) { _y = y; }
    void set_width (unsigned int w) { _w = w; }
    void set_height(unsigned int h) { _h = h; }
  private:
    unsigned _x;
    unsigned _y;
    unsigned _w;
    unsigned _h;
};

typedef struct NewRequest {
    std::string appid;
    std::string uuid;
    unsigned surface_id;
} NewRequest;

class MyHandler {
  public:
    MyHandler() {}
    ~MyHandler() = default;

    using sync_draw_handler = std::function<void(const char*, const char*, Rect)>;
    using reply_handler = std::function<void(json_object*)>;
    using new_request_handler = std::function<void(const NewRequest&)>;

    reply_handler on_reply;
    sync_draw_handler on_sync_draw;
    new_request_handler on_new_request;
};

class Binding
{
public:
    Binding();
    ~Binding();
    Binding(const Binding &) = delete;
    Binding &operator=(const Binding &) = delete;
    int init(int port, const std::string& token);
    void set_event_handler(const MyHandler& wmh);
    void subscribe_events();

    using handler_asyncSetSourceState = std::function<void(int sourceID, int handle)>;

    int call(const std::string& api, const std::string& verb, struct json_object* arg);

private:
    int run_eventloop();
    int init_event();
    int initialize_websocket();
    int dispatch_asyncSetSourceState(int sourceID, int handle, const std::string& sourceState);

    void (*onEvent)(const std::string& event, struct json_object* event_contents);
    void (*onReply)(struct json_object* reply);
    void (*onHangup)(void);

    struct afb_wsj1* wsj1;
    struct afb_wsj1_itf minterface;
    sd_event* mploop;
    int mport;
    std::string mtoken;
    MyHandler _wmh;

public:
    /* Don't use/ Internal only */
    void on_hangup(void *closure, struct afb_wsj1 *wsj);
    void on_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg);
    void on_event(void *closure, const char *event, struct afb_wsj1_msg *msg);
    void on_reply(void *closure, struct afb_wsj1_msg *msg);
};

#endif /* BINDING_H */
