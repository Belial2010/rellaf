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
//
// mybatis like sql dao framework
//

#include "dao.h"

namespace rellaf {

rellaf_enum_def(Dao::Charset);

void DaoResultList::push(const DaoResultRow& row) {
    _datas.emplace_back(row);
}

void DaoResultList::push(DaoResultRow&& row) {
    _datas.emplace_back(row);
}

size_t DaoResultList::size() const {
    return _datas.size();
}

bool DaoResultList::empty() const {
    return _datas.empty();
}

DaoResultRow& DaoResultList::front() {
    return _datas.front();
}

const DaoResultRow& DaoResultList::front() const {
    return _datas.front();
}

DaoResultRow& DaoResultList::back() {
    return _datas.back();
}

const DaoResultRow& DaoResultList::back() const {
    return _datas.back();
}

const DaoResultRow& DaoResultList::get(size_t idx) const {
    return _datas[idx];
}

const DaoResultRow& DaoResultList::operator[](size_t idx) const {
    return _datas[idx];
}

std::deque<DaoResultRow>::const_iterator DaoResultList::begin() const {
    return _datas.begin();
}

std::deque<DaoResultRow>::const_iterator DaoResultList::end() const {
    return _datas.end();
}

void Dao::split_section(const std::string& section_str, std::deque<std::string>& sections) {
    sections.clear();
    if (section_str.empty()) {
        return;
    }

    char* begin = const_cast<char*>(section_str.c_str());
    do {
        const char* pos = strchr(begin, '.');
        if (pos != nullptr) {
            if (pos == begin) {
                begin = const_cast<char*>(pos + 1);
                continue;
            }
            sections.emplace_back(std::string(begin, pos - begin));
            begin = const_cast<char*>(pos + 1);
        } else {
            sections.emplace_back(std::string(section_str, begin - section_str.c_str()));
            break;
        }
    } while (*begin != '\0');
}

bool Dao::get_plain_val_str(const Model* model, const std::string& key, std::string& val,
        bool& need_quote, bool& need_escape) {
    switch (model->rellaf_type().code) {
        case ModelTypeEnum::INT16_code:
        case ModelTypeEnum::INT_code:
        case ModelTypeEnum::INT64_code:
        case ModelTypeEnum::UINT16_code:
        case ModelTypeEnum::UINT32_code:
        case ModelTypeEnum::UINT64_code:
        case ModelTypeEnum::BOOL_code:
        case ModelTypeEnum::FLOAT_code:
        case ModelTypeEnum::DOUBLE_code:
            need_quote = false;
            need_escape = false;
            break;
        case ModelTypeEnum::CHAR_code:
        case ModelTypeEnum::STR_code:
            need_quote = true;
            need_escape = true;
            return true;
        default:
            return false;
    }
    val = model->str();
    return true;
}

bool Dao::get_plain_val(const Model* model, const std::deque<std::string>& sections,
        std::string& val, bool& need_quote, bool& need_escape) {
    val.clear();
    if (sections.empty() || model == nullptr) {
        return false;
    }

    if (model->rellaf_type() != ModelTypeEnum::e().OBJECT) {
        return false;
    }

    Object* travel = (Object*)const_cast<Model*>(model); // for travel only, so we need non const
    List* list = nullptr;
    for (auto& section : sections) {
        if (section.front() == '[' && section.back() == ']') {
            if (list == nullptr) { // current not list
                return false;
            }

            size_t idx = strtoul(section.c_str() + 1, nullptr, 10);
            travel = list->at<Object>(idx);

            continue;
        }

        if (travel->is_plain_member(section)) { // plain type just return;
            return get_plain_val_str(travel, section, val, need_quote, need_escape);

        } else if (travel->is_object_member(section)) {
            Object* temp = travel->get_object(section);
            if (temp == nullptr) {
                RELLAF_DEBUG("key %s is not object", section.c_str());
                return false;
            }
            list = nullptr;
            travel = temp;
        } else if (travel->is_list_member(section)) {
            list = &(travel->get_list(section));
        } else {
            RELLAF_DEBUG("key %s is not list", section.c_str());
            return false;
        }
    }

    // last section MUST be plain, and should be returned in for loop
    RELLAF_DEBUG("section %s invalid, last not value", sections.back().c_str());
    return false;
}

bool Dao::get_list_val(const Model* model, const std::deque<std::string>& sections,
        std::deque<std::string>& vals) {
    vals.clear();
    if (sections.empty() || model == nullptr) {
        return false;
    }

    Model* travel = const_cast<Model*>(model);
    List* list = nullptr;
    for (auto& section : sections) {
        if (section.front() == '<' && section.back() == '>') {
            if (list == nullptr) { // current not list
                return false;
            }

            size_t idx = strtoul(section.c_str() + 1, nullptr, 10);
            travel = list->at(idx); // never modify mem here,just force convert
            continue;
        }

        if (travel->rellaf_type() != ModelTypeEnum::e().LIST &&
                travel->rellaf_type() != ModelTypeEnum::e().OBJECT) {
            RELLAF_DEBUG("key %s should not be plain", section.c_str());
            return false;
        }

        if (travel->rellaf_type() == ModelTypeEnum::e().OBJECT) {
            Object* temp = ((Object*)travel)->get_object(section);
            if (temp == nullptr) {
                RELLAF_DEBUG("key %s is not object", section.c_str());
                return false;
            }
            list = nullptr;
            travel = temp;
        } else if (travel->rellaf_type() == ModelTypeEnum::e().LIST) {
            list = &(((Object*)travel)->get_list(section));
        } else {
            RELLAF_DEBUG("key %s is not list", section.c_str());
            return false;
        }
    }

    if (list != nullptr) { // convert to array list
        for (const Model* m : *list) {
            if (travel->rellaf_type() != ModelTypeEnum::e().LIST &&
                    travel->rellaf_type() != ModelTypeEnum::e().OBJECT) {
                continue;
            }
            vals.emplace_back(m->str());
        }
        return true;
    }

    // last section MUST be list, and should be returned in for loop
    RELLAF_DEBUG("section %s invalid, last not list", sections.back().c_str());
    return false;
}

bool Dao::append_sql(std::string& sql, const std::string& val, bool need_quote, bool need_escape) {
    if (need_quote) {
        sql += '\'';
    }
    if (need_escape) {
        std::string escape_val;
        if (!SqlEscape::instance(_charset.name).escape_field(val, escape_val)) {
            RELLAF_DEBUG("escape val failed");
            return false;
        }
        sql += escape_val;
    } else {
        sql += val;
    }
    if (need_quote) {
        sql += '\'';
    }
    return true;
}

}