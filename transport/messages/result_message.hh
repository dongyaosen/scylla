
/*
 * Copyright (C) 2015 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "cql3/result_set.hh"
#include "cql3/statements/prepared_statement.hh"

#include "transport/messages/result_message_base.hh"
#include "transport/event.hh"

#include "core/shared_ptr.hh"
#include "core/sstring.hh"

namespace cql_transport {

namespace messages {

class result_message::prepared : public result_message {
private:
    cql3::statements::prepared_statement::checked_weak_ptr _prepared;
    ::shared_ptr<cql3::prepared_metadata> _metadata;
    ::shared_ptr<const cql3::metadata> _result_metadata;
protected:
    prepared(cql3::statements::prepared_statement::checked_weak_ptr prepared)
        : _prepared(std::move(prepared))
        , _metadata{::make_shared<cql3::prepared_metadata>(_prepared->bound_names, _prepared->partition_key_bind_indices)}
        , _result_metadata{extract_result_metadata(_prepared->statement)}
    { }
public:
    cql3::statements::prepared_statement::checked_weak_ptr& get_prepared() {
        return _prepared;
    }

    ::shared_ptr<cql3::prepared_metadata> metadata() const {
        return _metadata;
    }

    ::shared_ptr<const cql3::metadata> result_metadata() const {
        return _result_metadata;
    }

    class cql;
    class thrift;
private:
    static ::shared_ptr<const cql3::metadata> extract_result_metadata(::shared_ptr<cql3::cql_statement> statement) {
        return statement->get_result_metadata();
    }
};

class result_message::visitor {
public:
    virtual void visit(const result_message::void_message&) = 0;
    virtual void visit(const result_message::set_keyspace&) = 0;
    virtual void visit(const result_message::prepared::cql&) = 0;
    virtual void visit(const result_message::prepared::thrift&) = 0;
    virtual void visit(const result_message::schema_change&) = 0;
    virtual void visit(const result_message::rows&) = 0;
};

class result_message::visitor_base : public visitor {
public:
    void visit(const result_message::void_message&) override {};
    void visit(const result_message::set_keyspace&) override {};
    void visit(const result_message::prepared::cql&) override {};
    void visit(const result_message::prepared::thrift&) override {};
    void visit(const result_message::schema_change&) override {};
    void visit(const result_message::rows&) override {};
};

class result_message::void_message : public result_message {
public:
    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};

class result_message::set_keyspace : public result_message {
private:
    sstring _keyspace;
public:
    set_keyspace(const sstring& keyspace)
        : _keyspace{keyspace}
    { }

    const sstring& get_keyspace() const {
        return _keyspace;
    }

    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};


class result_message::prepared::cql : public result_message::prepared {
    bytes _id;
public:
    cql(const bytes& id, cql3::statements::prepared_statement::checked_weak_ptr p)
        : result_message::prepared(std::move(p))
        , _id{id}
    { }

    const bytes& get_id() const {
        return _id;
    }

    static const bytes& get_id(::shared_ptr<cql_transport::messages::result_message::prepared> prepared) {
        auto msg_cql = dynamic_pointer_cast<const messages::result_message::prepared::cql>(prepared);
        if (msg_cql == nullptr) {
            throw std::bad_cast();
        }
        return msg_cql->get_id();
    }

    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};

class result_message::prepared::thrift : public result_message::prepared {
    int32_t _id;
public:
    thrift(int32_t id, cql3::statements::prepared_statement::checked_weak_ptr prepared)
        : result_message::prepared(std::move(prepared))
        , _id{id}
    { }

    const int32_t get_id() const {
        return _id;
    }

    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};

class result_message::schema_change : public result_message {
private:
    shared_ptr<event::schema_change> _change;
public:
    schema_change(shared_ptr<event::schema_change> change)
        : _change{change}
    { }

    shared_ptr<event::schema_change> get_change() const {
        return _change;
    }

    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};

class result_message::rows : public result_message {
private:
    std::unique_ptr<cql3::result_set> _rs;
public:
    rows(std::unique_ptr<cql3::result_set> rs) : _rs(std::move(rs)) {}

    const cql3::result_set& rs() const {
        return *_rs;
    }

    virtual void accept(result_message::visitor& v) override {
        v.visit(*this);
    }
};

}

}
