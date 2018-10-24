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

afb::event new_request;

static void request_map(afb_req_t r) {
    AFB_DEBUG(__FUNCTION__);
    json_object *args, *jtype;
    afb::req req(r);

    args = req.json();
    if(!json_object_object_get_ex(args, "type", &jtype)) {
        req.fail("bad-request");
    }
    string type = json_object_get_string(jtype);
    // TODO request to local binding
    req.success();
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
}

const afb_verb_t verbs[] = {
    afb::verb("request_map", request_map, "request map with argument", AFB_SESSION_LOA_0),
    afb::verbend()
};

const afb_binding_t afbBindingExport =
    afb::binding("map_service", verbs, "map service surface", init, nullptr, on_event, false, preinit, nullptr);
