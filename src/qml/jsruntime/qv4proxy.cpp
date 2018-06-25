/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qv4proxy_p.h"
#include "qv4symbol_p.h"
#include "qv4jscall_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(ProxyObject);

void Heap::ProxyObject::init(const QV4::Object *target, const QV4::Object *handler)
{
    Object::init();
    ExecutionEngine *e = internalClass->engine;
    this->target.set(e, target->d());
    this->handler.set(e, handler->d());
}

ReturnedValue ProxyObject::get(const Managed *m, StringOrSymbol *name, bool *hasProperty)
{
    Scope scope(m);
    const ProxyObject *o = static_cast<const ProxyObject *>(m);
    if (!o->d()->handler)
        return scope.engine->throwTypeError();

    ScopedObject target(scope, o->d()->target);
    Q_ASSERT(target);
    ScopedObject handler(scope, o->d()->handler);
    ScopedValue trap(scope, handler->get(scope.engine->id_get()));
    if (scope.hasException())
        return Encode::undefined();
    if (trap->isUndefined())
        return target->get(name, hasProperty);
    if (!trap->isFunctionObject())
        return scope.engine->throwTypeError();
    if (hasProperty)
        *hasProperty = true;

    JSCallData cdata(scope, 3, nullptr, handler);
    cdata.args[0] = target;
    cdata.args[1] = name;
    cdata.args[2] = o->d(); // ### fix receiver handling

    ScopedValue trapResult(scope, static_cast<const FunctionObject *>(trap.ptr)->call(cdata));
    ScopedProperty targetDesc(scope);
    PropertyAttributes attributes = target->getOwnProperty(name->toPropertyKey(), targetDesc);
    if (attributes != Attr_Invalid && !attributes.isConfigurable()) {
        if (attributes.isData() && !attributes.isWritable()) {
            if (!trapResult->sameValue(targetDesc->value))
                return scope.engine->throwTypeError();
        }
        if (attributes.isAccessor() && targetDesc->value.isUndefined()) {
            if (!trapResult->isUndefined())
                return scope.engine->throwTypeError();
        }
   }
    return trapResult->asReturnedValue();
}

ReturnedValue ProxyObject::getIndexed(const Managed *m, uint index, bool *hasProperty)
{
    Scope scope(m);
    ScopedString name(scope, Primitive::fromUInt32(index).toString(scope.engine));
    return get(m, name, hasProperty);
}

bool ProxyObject::put(Managed *m, StringOrSymbol *name, const Value &value)
{
    Scope scope(m);
    const ProxyObject *o = static_cast<const ProxyObject *>(m);
    if (!o->d()->handler)
        return scope.engine->throwTypeError();

    ScopedObject target(scope, o->d()->target);
    Q_ASSERT(target);
    ScopedObject handler(scope, o->d()->handler);
    ScopedValue trap(scope, handler->get(scope.engine->id_set()));
    if (scope.hasException())
        return Encode::undefined();
    if (trap->isUndefined())
        return target->put(name, value);
    if (!trap->isFunctionObject())
        return scope.engine->throwTypeError();

    JSCallData cdata(scope, 4, nullptr, handler);
    cdata.args[0] = target;
    cdata.args[1] = name;
    cdata.args[2] = value;
    cdata.args[3] = o->d(); // ### fix receiver handling

    ScopedValue trapResult(scope, static_cast<const FunctionObject *>(trap.ptr)->call(cdata));
    if (!trapResult->toBoolean())
        return false;
    ScopedProperty targetDesc(scope);
    PropertyAttributes attributes = target->getOwnProperty(name->toPropertyKey(), targetDesc);
    if (attributes != Attr_Invalid && !attributes.isConfigurable()) {
        if (attributes.isData() && !attributes.isWritable()) {
            if (!value.sameValue(targetDesc->value))
                return scope.engine->throwTypeError();
        }
        if (attributes.isAccessor() && targetDesc->set.isUndefined())
            return scope.engine->throwTypeError();
    }
    return true;
}

bool ProxyObject::putIndexed(Managed *m, uint index, const Value &value)
{
    Scope scope(m);
    ScopedString name(scope, Primitive::fromUInt32(index).toString(scope.engine));
    return put(m, name, value);
}

bool ProxyObject::deleteProperty(Managed *m, StringOrSymbol *name)
{
    Scope scope(m);
    const ProxyObject *o = static_cast<const ProxyObject *>(m);
    if (!o->d()->handler)
        return scope.engine->throwTypeError();

    ScopedObject target(scope, o->d()->target);
    Q_ASSERT(target);
    ScopedObject handler(scope, o->d()->handler);
    ScopedString deleteProp(scope, scope.engine->newString(QStringLiteral("deleteProperty")));
    ScopedValue trap(scope, handler->get(deleteProp));
    if (scope.hasException())
        return Encode::undefined();
    if (trap->isUndefined())
        return target->deleteProperty(name);
    if (!trap->isFunctionObject())
        return scope.engine->throwTypeError();

    JSCallData cdata(scope, 3, nullptr, handler);
    cdata.args[0] = target;
    cdata.args[1] = name;
    cdata.args[2] = o->d(); // ### fix receiver handling

    ScopedValue trapResult(scope, static_cast<const FunctionObject *>(trap.ptr)->call(cdata));
    if (!trapResult->toBoolean())
        return false;
    ScopedProperty targetDesc(scope);
    PropertyAttributes attributes = target->getOwnProperty(name->toPropertyKey(), targetDesc);
    if (attributes == Attr_Invalid)
        return true;
    if (!attributes.isConfigurable())
        return scope.engine->throwTypeError();
    return true;
}

bool ProxyObject::deleteIndexedProperty(Managed *m, uint index)
{
    Scope scope(m);
    ScopedString name(scope, Primitive::fromUInt32(index).toString(scope.engine));
    return deleteProperty(m, name);
}

//ReturnedValue ProxyObject::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
//{

//}

//ReturnedValue ProxyObject::call(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
//{

//}

DEFINE_OBJECT_VTABLE(Proxy);

void Heap::Proxy::init(QV4::ExecutionContext *ctx)
{
    Heap::FunctionObject::init(ctx, QStringLiteral("Proxy"));

    Scope scope(ctx);
    Scoped<QV4::Proxy> ctor(scope, this);
    ctor->defineDefaultProperty(QStringLiteral("revocable"), QV4::Proxy::method_revocable, 2);
}

ReturnedValue Proxy::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
{
    Scope scope(f);
    if (argc < 2 || !argv[0].isObject() || !argv[1].isObject())
        return scope.engine->throwTypeError();

    const Object *target = static_cast<const Object *>(argv);
    const Object *handler = static_cast<const Object *>(argv + 1);
    if (const ProxyObject *ptarget = target->as<ProxyObject>())
        if (!ptarget->d()->handler)
            return scope.engine->throwTypeError();
    if (const ProxyObject *phandler = handler->as<ProxyObject>())
        if (!phandler->d()->handler)
            return scope.engine->throwTypeError();

    ScopedObject o(scope, scope.engine->memoryManager->allocate<ProxyObject>(target, handler));
    return o->asReturnedValue();
}

ReturnedValue Proxy::call(const FunctionObject *f, const Value *, const Value *, int)
{
    return f->engine()->throwTypeError();
}

ReturnedValue Proxy::method_revocable(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    Scope scope(f);
    ScopedObject proxy(scope, Proxy::callAsConstructor(f, argv, argc));
    if (scope.hasException())
        return Encode::undefined();

    ScopedString revoke(scope, scope.engine->newString(QStringLiteral("revoke")));
    ScopedFunctionObject revoker(scope, createBuiltinFunction(scope.engine, revoke, method_revoke, 0));
    revoker->defineDefaultProperty(scope.engine->symbol_revokableProxy(), proxy);

    ScopedObject o(scope, scope.engine->newObject());
    ScopedString p(scope, scope.engine->newString(QStringLiteral("proxy")));
    o->defineDefaultProperty(p, proxy);
    o->defineDefaultProperty(revoke, revoker);
    return o->asReturnedValue();
}

ReturnedValue Proxy::method_revoke(const FunctionObject *f, const Value *, const Value *, int)
{
    Scope scope(f);
    Scoped<ProxyObject> proxy(scope, f->get(scope.engine->symbol_revokableProxy()));
    Q_ASSERT(proxy);

    proxy->d()->target.set(scope.engine, nullptr);
    proxy->d()->handler.set(scope.engine, nullptr);
    return Encode::undefined();
}