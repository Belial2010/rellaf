// Copyright 2018 Fankux
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Fankux (fankux@gmail.com)
//

#pragma once

#include <unordered_map>
#include <unordered_set>
#include "google/protobuf/service.h"
#include "brpc/server.h"
#include "brpc/http_method.h"
#include "common.h"
#include "model.h"
#include "function_mapper.hpp"

namespace rellaf {

using ::google::protobuf::RpcController;
using ::google::protobuf::Service;
using ::google::protobuf::Message;
using ::google::protobuf::Closure;

using brpc::HttpHeader;
using brpc::HttpMethod;

class BrpcService {
RELLAF_AVOID_COPY(BrpcService);

public:
    BrpcService() = default;

    virtual ~BrpcService() = default;

    /**
     * http request entry,
     * baidu rpc service implementation,
     * "controller" contain http protocol context
     */
    void entry(RpcController* controller, Message* request, Message* response, Closure* done);

    const std::unordered_map<std::string, std::string>& get_api_sign_mapper() const {
        return _api_sign_mapper;
    }

protected:
    virtual void bind_api_sign(const std::string& sign, const std::string& api);

    virtual void bind_pb_serivce(Service* service) = 0;

private:
    void return_response(brpc::Controller* cntl, const std::string& raw);

protected:
    std::unordered_set<std::string> _sign_entrys;
    std::unordered_map<std::string, std::string> _api_sign_mapper;
};

#define rellaf_brpc_http_dcl(_clazz_, _pb_req_t_, _pb_resp_t_)                                  \
public:                                                                                         \
using pb_req_t = _pb_req_t_;                                                                    \
using pb_resp_t = _pb_resp_t_;                                                                  \
void bind_pb_serivce(::google::protobuf::Service* base_service) override {                      \
    for (int i = 0; i < _clazz_::descriptor()->method_count(); ++i) {                           \
        const ::google::protobuf::MethodDescriptor* method = _clazz_::descriptor()->method(i);  \
        RELLAF_DEBUG("protobuf service :%s, methd: %s",                                         \
                _clazz_::descriptor()->name().c_str(), method->name().c_str());                 \
        _sign_entrys.insert(method->name());                                                    \
    }                                                                                           \
}                                                                                               \
private:                                                                                        \
class Reg {                                                                                     \
public:                                                                                         \
    Reg(_clazz_* inst, const std::string& sign, const std::string& api,                         \
            const std::string& name, HttpMethod method,                                         \
            std::function<int(HttpContext&, const std::string&, std::string&)> func) {          \
        inst->bind_api_sign(sign, api);                                                         \
        FunctionMapper::instance().reg(api, name, method, func);                                \
    }                                                                                           \
}

// definition brpc request entry method signature fowarding call BrpcService::entry
#define RELLAF_BRPC_HTTP_DEF_SIGN(_sign_)                                                       \
public:                                                                                         \
void _sign_(RpcController* controller, const pb_req_t* request,                                 \
        pb_resp_t* response, Closure* done) override {                                          \
    BrpcService::entry(controller, (Message*)request, (Message*)response, done);                \
}

#define rellaf_brpc_http_def_api(_sign_, _api_, _func_, _Ret_, _Param_, _PathVar_, _ReqBody_)  \
RELLAF_BRPC_HTTP_DEF_SIGN(_sign_)                                                               \
private:                                                                                        \
    static_assert(std::is_base_of<Model, _Ret_>::value, #_Ret_" not Model");                    \
    static_assert(std::is_base_of<Object, _Param_>::value, #_Param_" not Object");              \
    static_assert(std::is_base_of<Object, _PathVar_>::value, #_PathVar_" not Object");          \
    static_assert(std::is_base_of<Model, _ReqBody_>::value, #_ReqBody_" not Model");            \
    Reg _reg_##_sign_##_post_##_func_{this, #_sign_, _api_,                                     \
        #_sign_"-POST-"#_func_"-"#_ReqBody_, HttpMethod::HTTP_METHOD_POST,                      \
        [this] (HttpContext& ctx, const std::string& body, std::string& ret_body) {             \
            bool s;                                                                             \
            _Param_ param;                                                                      \
            const brpc::URI& uri = ctx.request_header.uri();                                    \
            for (auto iter = uri.QueryBegin(); iter != uri.QueryEnd(); ++iter) {                \
                param.set_plain(iter->first, iter->second);                                     \
            }                                                                                   \
            _PathVar_ var;                                                                      \
            for (auto& entry : ctx.path_vars) {                                                 \
                var.set_plain(entry.first, entry.second);                                       \
            }                                                                                   \
            _ReqBody_ request;                                                                  \
            if (is_plain(&request)) {                                                           \
                s = ((Model*)&request)->set_parse(body);                                        \
            } else {                                                                            \
                s = json_to_model(body, &request);                                              \
            }                                                                                   \
            if (!s) {                                                                           \
                return -1;                                                                      \
            }                                                                                   \
            _Ret_ ret = _func_(ctx, param, var, request);                                       \
            if (is_plain(&ret)) {                                                               \
                ret_body = ((Model*)&ret)->str();                                               \
            } else {                                                                            \
                if (!model_to_json(&ret, ret_body)) {                                           \
                    return -1;                                                                  \
                }                                                                               \
            }                                                                                   \
            return 0;                                                                           \
        }                                                                                       \
    };                                                                                          \
public:                                                                                         \
    _Ret_ _func_(HttpContext& context, const _Param_& params, const _PathVar_& vars,             \
            const _ReqBody_& request)

#define rellaf_brpc_http_def_post_ctx(_sign_, _api_, _func_, _Ret_, _Req_)                      \
RELLAF_BRPC_HTTP_DEF_SIGN(_sign_)                                                               \
private:                                                                                        \
    static_assert(std::is_base_of<Model, _Ret_>::value, #_Ret_" not Model");                    \
    static_assert(std::is_base_of<Model, _Req_>::value, #_Req_" not Model");                    \
    Reg _reg_##_sign_##_post_##_func_{this, #_sign_, _api_,                                     \
        #_sign_"-POST-"#_func_"-"#_Req_, HttpMethod::HTTP_METHOD_POST,                          \
        [this] (HttpContext& ctx, const std::string& body, std::string& ret_body) {             \
            bool s;                                                                             \
            _Req_ request;                                                                      \
            if (is_plain(&request)) {                                                           \
                s = ((Model*)&request)->set_parse(body);                                        \
            } else {                                                                            \
                s = json_to_model(body, &request);                                              \
            }                                                                                   \
            if (!s) {                                                                           \
                return -1;                                                                      \
            }                                                                                   \
            _Ret_ ret = _func_(ctx, request);                                                   \
            if (is_plain(&ret)) {                                                               \
                ret_body = ((Model*)&ret)->str();                                               \
            } else {                                                                            \
                if (!model_to_json(&ret, ret_body)) {                                           \
                    return -1;                                                                  \
                }                                                                               \
            }                                                                                   \
            return 0;                                                                           \
        }                                                                                       \
    };                                                                                          \
public:                                                                                         \
    _Ret_ _func_(HttpContext& context, const _Req_& request)

#define rellaf_brpc_http_def_post(_sign_, _api_, _func_, _Ret_, _Req_)                          \
private:                                                                                        \
    rellaf_brpc_http_def_post_ctx(_sign_, _api_, _func_, _Ret_, _Req_) {                        \
        return std::forward<_Ret_>(_func_(request));                                            \
    }                                                                                           \
public:                                                                                         \
    _Ret_ _func_(const _Req_& request)

} // namespace
