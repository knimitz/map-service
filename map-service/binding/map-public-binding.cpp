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

static string my_role = "Map.";
static int service_id = 0;
static unordered_map<string, shared_ptr<MapClient>> client_list;

static void request_map(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    char* error, *info;
    int surface = -1;
    bool ret = false;
    string service = my_role + std::to_string(++service_id);
    json_object *args, *wm_arg, *resp, *jsurface, *juuid;
    afb::req req(r);

    args = req.json();
    wm_arg = json_object_new_object();

    // Call window manager verb to attach service surface to the caller
    char* app_id = req.get_application_id();
    json_object_object_add(wm_arg, "destination", json_object_new_string(app_id));
    json_object_object_add(wm_arg, "service_surface", json_object_new_string(service.c_str()));
    // If UI process with this security context uses ivi surface id,
    // add "request_surface_id" parameter to the argument
    json_object_object_add(wm_arg, "request_surface_id", json_object_new_boolean(true));

    ret = afb::callsync("windowmanager", "attachSurfaceToApp", wm_arg, resp, error, info);

    // Unpack response from WM
    if(json_object_object_get_ex(resp, "surface", &jsurface) &&
       json_object_object_get_ex(resp, "uuid", &juuid)) {
        surface = json_object_get_int(jsurface);
    }
    else {
        req.fail();
        return;
    }

    // Request the UI process to create surface
    ret = afb::callsync("map_private", "request_map", args, resp, error, info);
    if(ret) {
        req.success();
        if(client_list.count(app_id) == 0) {
            shared_ptr<MapClient> client = std::make_shared<MapClient>();
            client_list[app_id] = client;
        }
    }
    else {
        req.fail();
    }
}

static void on_map_created(const char* app, const char* uuid) {
    // Get client object
    auto client = client_list[app];
    json_object* resp = json_object_new_object();
    json_object_object_add(resp, "map_surface", json_object_new_string(uuid));
    // afb::req req = client->get_req(uuid);
    // req.success(resp);
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
    AFB_NOTICE(__FUNCTION__);
    json_object *juuid, *jappid;
    if(string(event) == "map_created")
    {
        if(json_object_object_get_ex(object, "uuid", &juuid) &&
           json_object_object_get_ex(object, "appid", &jappid)) {
            const char* appid = json_object_get_string(jappid);
            const char* surface_uuid = json_object_get_string(juuid);
            on_map_created(appid, surface_uuid);
        }
    }
}

const afb_verb_t verbs[] = {
    afb::verb("request_map", request_map, "request map with argument", AFB_SESSION_LOA_0),
    afb::verbend()
};

const afb_binding_t afbBindingExport =
    afb::binding("map-service", verbs, "map service surface", init, nullptr, on_event, false, preinit, nullptr);
