/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string>
#include <json-c/json.h>
#include <memory>
#include <unordered_map>
#include "map-client.h"

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding>

using std::string;
using std::unordered_map;
using std::shared_ptr;

static const char _mp_prv_api[] = "map-private";
static const char _verb_req_map[] = "request_map";
static const char _key_appid[] = "appid";
static const char _key_uuid[] = "uuid";
static const char _key_mp_sfc[] = "map_surface";
static const char _ev_map_created[] = "map_created";
static unordered_map<string, shared_ptr<MapClient>> _client_list;

static void request_map(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    char *app_id, *error, *info;
    json_object *args, *resp, *jsurface, *juuid;
    afb::req req(r);
    args = req.json();
    app_id = req.get_application_id();
    json_object_object_add(args, _key_appid, json_object_new_string(app_id));
    json_object_get(args); // +1 for reference to json_object

    afb::callsync(_mp_prv_api, _verb_req_map, args, resp, error, info);
    AFB_INFO("%s : %s", error, info);
    AFB_INFO("%s", json_object_get_string(resp));

    req.success();
}

static void subscribe(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    afb::req req(r);
    char* app_id = req.get_application_id();
    if(_client_list.count(app_id) == 0) {
        shared_ptr<MapClient> client = std::make_shared<MapClient>(req);
        _client_list[app_id] = client;
    }
    if(app_id) {
        free(app_id);
    }
}

static void on_map_created(const char* app, const char* uuid) {
    // Get client object
    AFB_DEBUG(__FUNCTION__);
    auto client = _client_list[app];
    if(client != nullptr) {
        json_object* resp = json_object_new_object();
        json_object_object_add(resp, _key_mp_sfc, json_object_new_string(uuid));
        client->push_map_created(resp);
        /* afb::req req = client->get_req(uuid);
        req.success(resp); */
    }
    else {
        AFB_WARNING("client not found");
    }
}

int preinit(afb_api_t api) {
    AFB_NOTICE(__FUNCTION__);
    return 0;
}

int init(afb_api_t api) {
    AFB_NOTICE(__FUNCTION__);
    return 0;
}

void on_event(afb_api_t api, const char* event, struct json_object *object)
{
    AFB_DEBUG(__FUNCTION__);
    AFB_INFO("%s, %s", event, json_object_get_string(object));
    if(string(event).find(_ev_map_created) != string::npos)
    {
        json_object *juuid, *jappid;
        if(json_object_object_get_ex(object, _key_uuid, &juuid) &&
           json_object_object_get_ex(object, _key_appid, &jappid)) {
            const char* appid = json_object_get_string(jappid);
            const char* surface_uuid = json_object_get_string(juuid);
            on_map_created(appid, surface_uuid);
        }
    }
}

const afb_verb_t verbs[] = {
    afb::verb("request_map", request_map, "request map with argument", AFB_SESSION_LOA_0),
    afb::verb("subscribe", subscribe, "subscribe event", AFB_SESSION_LOA_0),
    afb::verbend()
};

const afb_binding_t afbBindingExport =
    afb::binding("map-service", verbs, "map service surface", init, nullptr, on_event, false, preinit, nullptr);
