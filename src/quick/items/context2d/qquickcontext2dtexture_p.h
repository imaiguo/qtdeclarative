/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCONTEXT2DTEXTURE_P_H
#define QQUICKCONTEXT2DTEXTURE_P_H

#include <QtQuick/qsgtexture.h>
#include "qquickcanvasitem_p.h"
#include "qquickcontext2d_p.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickContext2DTile;
class QQuickContext2DCommandBuffer;

class QQuickContext2DTexture : public QSGDynamicTexture
{
    Q_OBJECT
public:
    QQuickContext2DTexture();
    ~QQuickContext2DTexture();

    virtual bool hasAlphaChannel() const {return true;}
    virtual bool hasMipmaps() const {return false;}
    virtual QSize textureSize() const;
    virtual void lock() {}
    virtual void unlock() {}
    virtual void wait() {}
    virtual void wake() {}
    bool threadRendering() const {return m_threadRendering;}
    virtual bool supportThreadRendering() const = 0;
    virtual bool supportDirectRendering() const = 0;
    virtual QQuickCanvasItem::RenderTarget renderTarget() const = 0;
    virtual QImage toImage(const QRectF& region = QRectF()) = 0;
    static QRect tiledRect(const QRectF& window, const QSize& tileSize);

    virtual bool setCanvasSize(const QSize &size);
    virtual bool setTileSize(const QSize &size);
    virtual bool setCanvasWindow(const QRect& canvasWindow);
    void setSmooth(bool smooth);
    bool setDirtyRect(const QRect &dirtyRect);
    virtual void canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth);
    bool canvasDestroyed();
Q_SIGNALS:
    void textureChanged();

public Q_SLOTS:
    void markDirtyTexture();
    void setItem(QQuickCanvasItem* item);
    void paint();

protected:
    void paintWithoutTiles();
    virtual QPaintDevice* beginPainting() {m_painting = true; return 0; }
    virtual void endPainting() {m_painting = false;}
    virtual QQuickContext2DTile* createTile() const = 0;
    virtual void compositeTile(QQuickContext2DTile* tile) = 0;

    void clearTiles();
    QRect createTiles(const QRect& window);

    QList<QQuickContext2DTile*> m_tiles;
    QQuickContext2D* m_context;

    QQuickContext2D::State m_state;

    QQuickCanvasItem* m_item;
    QSize m_canvasSize;
    QSize m_tileSize;
    QRect m_canvasWindow;

    uint m_dirtyCanvas : 1;
    uint m_dirtyTexture : 1;
    uint m_threadRendering : 1;
    uint m_smooth : 1;
    uint m_tiledCanvas : 1;
    uint m_doGrabImage : 1;
    uint m_painting : 1;
};

class QQuickContext2DFBOTexture : public QQuickContext2DTexture
{
    Q_OBJECT

public:
    QQuickContext2DFBOTexture();
    ~QQuickContext2DFBOTexture();
    virtual int textureId() const;
    virtual bool updateTexture();
    virtual QQuickContext2DTile* createTile() const;
    virtual QImage toImage(const QRectF& region = QRectF());
    virtual QPaintDevice* beginPainting();
    virtual void endPainting();
    QRectF normalizedTextureSubRect() const;
    virtual bool supportThreadRendering() const {return false;}
    virtual bool supportDirectRendering() const {return false;}
    virtual QQuickCanvasItem::RenderTarget renderTarget() const;
    virtual void compositeTile(QQuickContext2DTile* tile);
    virtual void bind();
    virtual bool setCanvasSize(const QSize &size);
    virtual bool setTileSize(const QSize &size);
    virtual bool setCanvasWindow(const QRect& canvasWindow);
private Q_SLOTS:
    void grabImage();

private:
    bool doMultisampling() const;
    QImage m_grabedImage;
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLFramebufferObject *m_multisampledFbo;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QSize m_fboSize;
    QPaintDevice *m_paint_device;
};

class QSGPlainTexture;
class QQuickContext2DImageTexture : public QQuickContext2DTexture
{
    Q_OBJECT

public:
    QQuickContext2DImageTexture(bool threadRendering = true);
    ~QQuickContext2DImageTexture();
    virtual int textureId() const;
    virtual void bind();
    virtual bool supportThreadRendering() const {return true;}
    virtual bool supportDirectRendering() const;
    virtual QQuickCanvasItem::RenderTarget renderTarget() const;
    virtual void lock();
    virtual void unlock();
    virtual void wait();
    virtual void wake();

    virtual bool updateTexture();
    virtual QQuickContext2DTile* createTile() const;
    virtual QImage toImage(const QRectF& region = QRectF());
    virtual QPaintDevice* beginPainting();
    virtual void compositeTile(QQuickContext2DTile* tile);

private Q_SLOTS:
    void grabImage(const QRect& r);
private:
    QImage m_image;
    QImage m_grabedImage;
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    QPainter m_painter;
    QSGPlainTexture*  m_texture;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DTEXTURE_P_H
