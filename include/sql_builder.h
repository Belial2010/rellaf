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
// java mybatis like sql dao framework
//

#pragma once

#include <string.h>
#include <map>
#include <deque>
#include <memory>
#include "common.h"
#include "var_pattern.h"
#include "model.h"
#include "model_type.h"
#include "mysql_escape.h"

namespace rellaf {

class SqlResultRow {
public:
    void set(const std::string& name, const std::string& val) {
        _row.emplace(name, val);
    }

    void set(std::string&& name, std::string&& val) {
        _row.emplace(name, val);
    }

    const std::map<std::string, std::string>& row() const {
        return _row;
    }

private:
    std::map<std::string, std::string> _row;
};

class SqlResultList {
public:
    friend class SqlBuilder;

    void push(const SqlResultRow& row);

    void push(SqlResultRow&& row);

    size_t size() const;

    bool empty() const;

    SqlResultRow& front();

    const SqlResultRow& front() const;

    SqlResultRow& back();

    const SqlResultRow& back() const;

    const SqlResultRow& get(size_t idx) const;

    const SqlResultRow& operator[](size_t idx) const;

    std::deque<SqlResultRow>::const_iterator begin() const;

    std::deque<SqlResultRow>::const_iterator end() const;

private:
    std::deque<SqlResultRow> _datas;
};

class SqlBuilder {

public:
    class Charset : public IEnum {
    rellaf_enum_dcl(Charset);

    rellaf_enum_item_def(1, UTF8);
    rellaf_enum_item_def(2, GBK);
    };

    typedef EnumItem CharsetType;

public:
    void set_charset(const CharsetType& charset) {
        _charset = charset;
    }

protected:
    class Reg {
    public:
        Reg(SqlBuilder* inst, const std::string& method, const std::string& pattern) {

            std::deque<SqlPattern::Stub> pices;
            PatternErr err;
            if (!SqlPattern::explode(pattern, pices, err)) {
                RELLAF_DEBUG("reg sql failed, explode pattern failed : %s", pattern.c_str());
                exit(-1);
            }

            inst->_patterns.emplace(method, pattern);
            inst->_pices.emplace(method, pices);
        }
    };

    template<class T>
    void build_dao_models(std::map<std::string, const Model*>& models, const T& arg) {
        if (!std::is_base_of<Model, T>::value) {
            return;
        }
        if (arg.rellaf_tags().empty()) {
            models.emplace("", &arg);
            return;
        }
        for (auto& tag : arg.rellaf_tags()) {
            models.emplace(tag, &arg);
        }
    }

    template<class ...Args>
    bool prepare_statement(const std::string& method, std::string& sql, const Args& ...args) {
        sql.clear();

        auto entry = _pices.find(method);
        if (entry == _pices.end()) {
            return false;
        }

        int arg_count = sizeof...(args);
        bool is_multi_arg = arg_count > 1;

        std::map<std::string, const Model*> models;
        bool arr[] = {(build_dao_models(models, args), true)...}; // for arguments expansion
        (void)(arr);// suppress warning

        for (auto& m_entry : models) {
            RELLAF_DEBUG("%s ==> %s", m_entry.first.c_str(),
                    m_entry.second->debug_str().c_str());
        }

        for (const SqlPattern::Stub& stub : entry->second) {
            std::deque<std::string> sections;
            if (stub.type() == SqlPattern::LITERAL) {
                sql += stub.value();
                continue;
            }

            split_section(stub.value(), sections);
            if (sections.empty()) {
                RELLAF_DEBUG("no section failed : %s", stub.value().c_str());
                return false;
            }

            if (models.empty()) {
                RELLAF_DEBUG("no available parameters");
                return false;
            }

            std::deque<const Model*> model_box;
            if (!is_multi_arg) {
                model_box.push_front(models.begin()->second);
            } else {
                auto model_entry = models.find(sections.front());
                if (model_entry == models.end()) {
                    RELLAF_DEBUG("no dao model name : %s", sections.front().c_str());
                    return false;
                }
                model_box.push_front(model_entry->second);
                sections.pop_front();
            }

            if (stub.type() == SqlPattern::FIELD) {
                std::string val;
                bool need_quote = false;
                bool need_escape = false;
                if (!get_plain_val(model_box.front(), sections, val, need_quote, need_escape)) {
                    return false;
                }
                if (!append_sql(sql, val, need_quote, need_escape)) {
                    return false;
                }
            } else if (stub.type() == SqlPattern::LIST) {
                std::deque<std::string> vals;
                if (!get_list_val(model_box.front(), sections, vals)) {
                    return false;
                }
                sql += '(';
                for (auto& val : vals) {
                    if (!append_sql(sql, val, true, true)) {
                        return false;
                    }
                    sql += ',';
                }
                if (sql.back() == ',') {
                    sql.back() = ')';
                } else {
                    sql += ')';
                }
            }
        }
        RELLAF_DEBUG("mapper select sql : %s", sql.c_str());
        return true;
    }

    template<class Ret, class ...Args>
    int select_impl(std::string* sql, const std::string& method, Ret& ret, const Args& ...args) {
        std::string sql_inner;
        if (sql == nullptr) {
            sql = &sql_inner;
        }
        if (!prepare_statement(method, *sql, args...)) {
            return -1;
        }
        if (sql == &sql_inner && _s_select_func) {
            SqlResultList ret_list;
            int retval = _s_select_func(*sql, ret_list);
            if (retval <= 0) {
                return retval;
            }

            for (const auto& entry : ret_list.front().row()) {
                if (ret.rellaf_type() == ModelTypeEnum::e().OBJECT) {
                    if (!((Object*)&ret)->set_plain(entry.first, entry.second)) {
                        RELLAF_DEBUG("select impl set result key %s failed", entry.first.c_str());
                        return -1;
                    }
                }
            }
            return retval;
        }
        return 0;
    }

    template<class Ret, class ...Args>
    int select_list_impl(std::string* sql, const std::string& method, std::deque<Ret>& ret_list,
            const Args& ...args) {
        std::string sql_inner;
        if (sql == nullptr) {
            sql = &sql_inner;
        }
        if (!prepare_statement(method, *sql, args...)) {
            return -1;
        }
        if (sql == &sql_inner && _s_select_func) {
            SqlResultList dao_ret_list;
            int retval = _s_select_func(*sql, dao_ret_list);
            if (retval <= 0) {
                return retval;
            }

            for (auto& item : dao_ret_list) {
                Ret ret;
                for (const auto& entry : item.row()) {
                    if (!ret.set(entry.first, entry.second)) {
                        RELLAF_DEBUG("select impl set result key %s failed", entry.first.c_str());
                        return -1;
                    }
                }
                ret_list.emplace_back(ret);
            }
            return retval;
        }
        return 0;
    }

protected:
    void split_section(const std::string& section_str, std::deque<std::string>& sections);

    bool get_plain_val_str(const Model* model, std::string& val,
            bool& need_quote, bool& need_escape);

    bool get_plain_val(const Model* model, const std::deque<std::string>& sections,
            std::string& val, bool& need_quote, bool& need_escape);

    bool get_list_val(const Model* model, const std::deque<std::string>& sections,
            std::deque<std::string>& vals);

    bool append_sql(std::string& sql, const std::string& val, bool need_quote, bool need_escape);

protected:
    std::function<int(const std::string&, SqlResultList&)> _s_select_func;

private:
    CharsetType _charset = Charset::e().UTF8;
    std::map<std::string, std::string> _patterns;
    std::map<std::string, std::deque<SqlPattern::Stub>> _pices;

};

#define rellaf_dao_select(_method_, _pattern_, _Ret_)                               \
public:                                                                             \
template<class ...Args> int _method_(_Ret_& ret, const Args& ...args) {             \
    return select_impl(nullptr, #_method_, ret, args...);                           \
}                                                                                   \
template<class ...Args> int _method_##_sql(std::string& sql, const Args& ...args) { \
    Void v;                                                                         \
    return select_impl(&sql, #_method_, v, args...);                                \
}                                                                                   \
private:                                                                            \
Reg _reg_##_method_{this, #_method_, _pattern_}

#define rellaf_dao_select_list(_method_, _pattern_, _Ret_)                          \
public:                                                                             \
template<class ...Args> int _method_(std::deque<_Ret_>& ret, const Args& ...args) { \
    return select_list_impl(nullptr, #_method_, ret, args...);                      \
}                                                                                   \
template<class ...Args> int _method_##_sql(std::string& sql, const Args& ...args) { \
    return select_list_impl(&sql, #_method_, {}, args...);                          \
}                                                                                   \
private:                                                                            \
Reg _reg_##_method_{this, #_method_, _pattern_}

}