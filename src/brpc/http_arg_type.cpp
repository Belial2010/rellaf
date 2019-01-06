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


#include "brpc/http_arg_type.h"

namespace rellaf {

rellaf_enum_def(HttpArgTypeEnum);

constexpr int HttpArgTypeEnum::REQ_BODY_code;
constexpr int HttpArgTypeEnum::REQ_PARAM_code;
constexpr int HttpArgTypeEnum::PATH_VAR_code;

} // namespace rellaf
