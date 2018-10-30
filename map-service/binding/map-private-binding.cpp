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

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding>

using std::string;

static int service_id = 0;
static string g_my_role = "Map.";
static const char _to_myself[] = "map-private";
static const char _key_dest[] = "destination";
static const char _key_srv_srfc[] = "service_surface";
static const char _key_req_srfc_id[] = "request_surface_id";
static const char _key_srfc[] = "surface";
static const char _key_uuid[] = "uuid";
static const char _key_appid[] = "appid";

static const char _api_wm[] = "windowmanager";
static const char _verb_wm_atch_srf_to_app[] = "attachSurfaceToApp";
static const char _verb_provide_surface[] = "provide_surface";

static bool g_first_time = true; // This will be deleted

afb::event new_request, map_created;

static void request_map(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    char* error, *info;
    json_object *args, *wm_arg, *j_app, *resp, *jsurface, *juuid;
    int surface = -1;
    string service = g_my_role + std::to_string(++service_id); // This may be deleted
    afb::req req(r);

    // Subscribe at first time
    /* const auto ctxt = req.context(); */
    if(g_first_time) {
        AFB_DEBUG("first time subscribe");
        req.subscribe(map_created);
        g_first_time = false;
    }

    args = req.json();
    wm_arg = json_object_new_object();

    // Call window manager verb to attach service surface to the caller
    json_object_object_get_ex(args, _key_appid, &j_app);
    const char* app_id = json_object_get_string(j_app);
    if(app_id == nullptr) {
        req.fail("application id is not set");
        return;
    }
    json_object_object_add(wm_arg, _key_dest, json_object_new_string(app_id));
    json_object_object_add(wm_arg, _key_srv_srfc, json_object_new_string(service.c_str())); // This may be deleted
    // If UI process in this security context requeires ivi surface id,
    // add "request_surface_id" parameter to the argument
    json_object_object_add(wm_arg, _key_req_srfc_id, json_object_new_boolean(true));

    AFB_DEBUG("request to wm: %s", json_object_get_string(wm_arg));
    bool ret = afb::callsync(_api_wm, _verb_wm_atch_srf_to_app, wm_arg, resp, error, info);

    AFB_INFO("error : %s, info: %s, resp: %s", error, info, json_object_get_string(resp));
    if(ret) {
        // afb returns true if calling verb is success
        req.fail("failed to call window manager verb");
        return;
    }

    // Unpack response from WM
    if(json_object_object_get_ex(resp, _key_srfc, &jsurface)) {
        surface = json_object_get_int(jsurface);
    }
    if(json_object_object_get_ex(resp, _key_uuid, &juuid)) {
        // Request the UI process to create surface
        const char* uuid = json_object_get_string(juuid);
        json_object* j_ui_req = json_object_new_object();
        // Add surface, uuid
        json_object_object_add(j_ui_req, _key_srfc, json_object_new_int(surface));
        json_object_object_add(j_ui_req, _key_uuid, json_object_new_string(uuid));
        // ========= Add some request to UI process here ===========

        // =================================================
        new_request.push(args);
        req.success();

        ////////////////// test ////////////////////////
        json_object *jtest, *jresponse_test;
        char* test_info, *test_err;
        jtest = json_object_new_object();
        json_object_object_add(jtest, _key_uuid, json_object_new_string(uuid));
        // This is test because I don't implement UI process, so UI process can't create surface now.
        afb::callsync(_to_myself, _verb_provide_surface, jtest, jresponse_test, test_info, test_err);
        AFB_INFO("response test : %s", json_object_get_string(jresponse_test));
        json_object_put(jresponse_test);
        ////////////////// test ////////////////////////
    }
    else {
        req.fail("window manager doesn't return uuid");
    }
    // free response json_object
    json_object_put(resp);



}

static void start_service(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    afb::req req(r);

    req.subscribe(new_request);
    /* if(json_object_object_get_ex(args, "type", jtype) && req.context() == nullptr) {
        string type = json_object_get_string(jtype);
        if(type == "local") {
            req.subscribe(map_created);
        }
    } */
    req.success();
}

static void stop_service(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    afb::req req(r);

    req.unsubscribe(new_request);
    req.success();

    // TODO : some shutdown processes are necessary.
}

static void provide_surface(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    afb::req req(r);

    // Store uuid
    json_object *j, *j_uuid;
    j = req.json();

    // Notify app of uuid with request id
    if(json_object_object_get_ex(j, "uuid", &j_uuid)) {
        map_created.push(j);
        req.success();
    }
    else {
        req.fail();
    }
}

int preinit(afb_api_t api) {
    AFB_NOTICE(__FUNCTION__);
    return 0;
}

int init(afb_api_t api) {
    AFB_NOTICE(__FUNCTION__);
    new_request = afb::make_event("new_request");
    map_created = afb::make_event("map_created");
    return 0;
}

void on_event(afb_api_t api, const char* event, struct json_object *object)
{
    AFB_NOTICE(__FUNCTION__);
}

const afb_verb_t verbs[] = {
    afb::verb("start_service", start_service, "start service", AFB_SESSION_LOA_0),
    afb::verb("stop_service", stop_service, "stop service", AFB_SESSION_LOA_0),
    afb::verb(_verb_provide_surface, provide_surface, "provide service", AFB_SESSION_LOA_0),
    afb::verb("request_map", request_map, "receive request from public", AFB_SESSION_LOA_0),
    afb::verbend()
};

const afb_binding_t afbBindingExport =
    afb::binding("map-private", verbs, "map service surface", init, nullptr, on_event, false, preinit, nullptr);
