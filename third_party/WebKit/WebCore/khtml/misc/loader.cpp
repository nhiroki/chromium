/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2002 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#undef CACHE_DEBUG
//#define CACHE_DEBUG
#include <assert.h>

#include "loader.h"

// up to which size is a picture for sure cacheable
#define MAXCACHEABLE 40*1024
// default cache size
#define DEFCACHESIZE 4096*1024

#include <qasyncio.h>
#include <qasyncimageio.h>
#include <qpainter.h>
#include <qbitmap.h>
#include <qmovie.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kglobal.h>
#include <kimageio.h>
#include <kcharsets.h>
#include <kiconloader.h>
#include <scheduler.h>
#include <kdebug.h>
#include "khtml_factory.h"
#include "khtml_part.h"

#include "html/html_documentimpl.h"
#include "css/css_stylesheetimpl.h"
#include "xml/dom_docimpl.h"

#if APPLE_CHANGES
#include "KWQAssertions.h"
#include "KWQLoader.h"
#endif

using namespace khtml;
using namespace DOM;

#if APPLE_CHANGES
static bool cacheDisabled;
#endif

CachedObject::~CachedObject()
{
    if(m_deleted) abort();
    Cache::removeFromLRUList(this);
    m_deleted = true;
#if APPLE_CHANGES
    KWQReleaseResponse(m_response);
#endif
}

void CachedObject::finish()
{
    if( m_size > MAXCACHEABLE )
        m_status = Uncacheable;
    else
        m_status = Cached;
    KURL url(m_url.string());
    if (m_expireDateChanged && url.protocol().startsWith("http"))
    {
        m_expireDateChanged = false;
        KIO::http_update_cache(url, false, m_expireDate);
#ifdef CACHE_DEBUG
        kdDebug(6060) << " Setting expire date for image "<<m_url.string()<<" to " << m_expireDate << endl;
#endif
    }
#ifdef CACHE_DEBUG
    else kdDebug(6060) << " No expire date for image "<<m_url.string()<<endl;
#endif
}

void CachedObject::setExpireDate(time_t _expireDate, bool changeHttpCache)
{
    if ( _expireDate == m_expireDate)
        return;

    if (m_status == Uncacheable || m_status == Cached)
    {
        finish();
    }
    m_expireDate = _expireDate;
    if (changeHttpCache && m_expireDate)
       m_expireDateChanged = true;
}

bool CachedObject::isExpired() const
{
    if (!m_expireDate) return false;
    time_t now = time(0);
    return (difftime(now, m_expireDate) >= 0);
}

#if APPLE_CHANGES

void CachedObject::setResponse(void *response)
{
    KWQRetainResponse(response);
    KWQReleaseResponse(m_response);
    m_response = response;
}

#endif

void CachedObject::setRequest(Request *_request)
{
    if ( _request && !m_request )
        m_status = Pending;
    m_request = _request;
    if (canDelete() && m_free)
        delete this;
    else if (allowInLRUList())
        Cache::insertInLRUList(this);
}

void CachedObject::ref(CachedObjectClient *c)
{
    m_clients.remove(c);
    m_clients.append(c);
    Cache::removeFromLRUList(this);
    increaseAccessCount();
}

void CachedObject::deref(CachedObjectClient *c)
{
    m_clients.remove(c);
    if (allowInLRUList())
        Cache::insertInLRUList(this);
}

void CachedObject::setSize(int size)
{
    bool sizeChanged = Cache::adjustSize(this, size - m_size);

    // The object must now be moved to a different queue, since its size has been changed.
    if (sizeChanged && allowInLRUList())
        Cache::removeFromLRUList(this);

    m_size = size;
    
    if (sizeChanged && allowInLRUList())
        Cache::insertInLRUList(this);
}

// -------------------------------------------------------------------------------------------

CachedCSSStyleSheet::CachedCSSStyleSheet(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate, const QString& charset)
    : CachedObject(url, CSSStyleSheet, _cachePolicy, _expireDate)
{
    // It's css we want.
    setAccept( QString::fromLatin1("text/css") );
    // load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    bool b;
    if(!charset.isEmpty())
	m_codec = KGlobal::charsets()->codecForName(charset, b);
    else
        m_codec = QTextCodec::codecForMib(4); // latin-1
}

CachedCSSStyleSheet::CachedCSSStyleSheet(const DOMString &url, const QString &stylesheet_data)
    : CachedObject(url, CSSStyleSheet, KIO::CC_Verify, 0, stylesheet_data.length())
{
    m_loading = false;
    m_status = Persistent;
    m_codec = 0;
    m_sheet = DOMString(stylesheet_data);
}


CachedCSSStyleSheet::~CachedCSSStyleSheet()
{
}

void CachedCSSStyleSheet::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->setStyleSheet( m_url, m_sheet );
}

void CachedCSSStyleSheet::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if ( canDelete() && m_free )
      delete this;
}

void CachedCSSStyleSheet::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());
    QString data = m_codec->toUnicode( buffer.buffer().data(), size() );
    m_sheet = DOMString(data);
    m_loading = false;

    checkNotify();
}

void CachedCSSStyleSheet::checkNotify()
{
    if(m_loading) return;

#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "CachedCSSStyleSheet:: finishedLoading " << m_url.string() << endl;
#endif

    QPtrList<CachedObjectClient> clients(m_clients);
    CachedObjectClient *c;
    for ( c = clients.first(); c != 0; c = clients.next() )
        c->setStyleSheet( m_url, m_sheet );
}


void CachedCSSStyleSheet::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}

// -------------------------------------------------------------------------------------------

CachedScript::CachedScript(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate, const QString& charset)
    : CachedObject(url, Script, _cachePolicy, _expireDate)
{
    // It's javascript we want.
    // But some websites think their scripts are <some wrong mimetype here>
    // and refuse to serve them if we only accept application/x-javascript.
    setAccept( QString::fromLatin1("*/*") );
    // load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    bool b;
    if(!charset.isEmpty())
        m_codec = KGlobal::charsets()->codecForName(charset, b);
    else
	m_codec = QTextCodec::codecForMib(4); // latin-1
}

CachedScript::CachedScript(const DOMString &url, const QString &script_data)
    : CachedObject(url, Script, KIO::CC_Verify, 0, script_data.length())
{
    m_loading = false;
    m_status = Persistent;
    m_codec = 0;
    m_script = DOMString(script_data);
}

CachedScript::~CachedScript()
{
}

void CachedScript::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->notifyFinished(this);
}

void CachedScript::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if ( canDelete() && m_free )
      delete this;
}

void CachedScript::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());
    QString data = m_codec->toUnicode( buffer.buffer().data(), size() );
    m_script = DOMString(data);
    m_loading = false;
    checkNotify();
}

void CachedScript::checkNotify()
{
    if(m_loading) return;

    QPtrList<CachedObjectClient> clients(m_clients);
    CachedObjectClient *c;
    for ( c = clients.first(); c != 0; c = clients.next() )
        c->notifyFinished(this);
}


void CachedScript::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}

// ------------------------------------------------------------------------------------------

#if !APPLE_CHANGES

namespace khtml
{

    class ImageSource : public QDataSource
    {
    public:
        ImageSource(QByteArray buf);

        /**
         * Overload QDataSource::readyToSend() and returns the number
         * of bytes ready to send if not eof instead of returning -1.
         */
        int readyToSend();

        /*!
          Reads and sends a block of data.
        */
        void sendTo(QDataSink*, int count);

        /**
         * Sets the EOF state.
         */
        void setEOF( bool state );

        /*!
          KHTMLImageSource's is rewindable.
        */
        bool rewindable() const;

        /*!
          Enables rewinding.  No special action is taken.
        */
        void enableRewind(bool on);

        /*
          Calls reset() on the QIODevice.
        */
        void rewind();

        /*
          Indicates that the buffered data is no longer
          needed.
        */
        void cleanBuffer();

        QByteArray buffer;
        unsigned int pos;
    private:
        bool eof     : 1;
        bool rew     : 1;
        bool rewable : 1;
    };
}


ImageSource::ImageSource(QByteArray buf)
{
  buffer = buf;
  rew = false;
  pos = 0;
  eof = false;
  rewable = true;
}

int ImageSource::readyToSend()
{
    if(eof && pos == buffer.size())
        return -1;

    return  buffer.size() - pos;
}

void ImageSource::sendTo(QDataSink* sink, int n)
{
    sink->receive((const uchar*)&buffer.at(pos), n);

    pos += n;

    // buffer is no longer needed
    if(eof && pos == buffer.size() && !rewable)
    {
        buffer.resize(0);
        pos = 0;
    }
}

void ImageSource::setEOF( bool state )
{
    eof = state;
}

// ImageSource's is rewindable.
bool ImageSource::rewindable() const
{
    return rewable;
}

// Enables rewinding.  No special action is taken.
void ImageSource::enableRewind(bool on)
{
    rew = on;
}

// Calls reset() on the QIODevice.
void ImageSource::rewind()
{
    pos = 0;
    if (!rew) {
        QDataSource::rewind();
    } else
        ready();
}

void ImageSource::cleanBuffer()
{
    // if we need to be able to rewind, buffer is needed
    if(rew)
        return;

    rewable = false;

    // buffer is no longer needed
    if(eof && pos == buffer.size())
    {
        buffer.resize(0);
        pos = 0;
    }
}

static QString buildAcceptHeader()
{
    QString result = KImageIO::mimeTypes( KImageIO::Reading ).join(", ");
    if (result.right(2) == ", ")
        result = result.left(result.length()-2);
    return result;
}

#endif // APPLE_CHANGES

static bool crossDomain(const QString &a, const QString &b)
{
    if (a == b) return false;

    QStringList l1 = QStringList::split('.', a);
    QStringList l2 = QStringList::split('.', b);

    while(l1.count() > l2.count())
        l1.pop_front();

    while(l2.count() > l1.count())
        l2.pop_front();

    while(l2.count() >= 2)
    {
        if (l1 == l2)
           return false;

        l1.pop_front();
        l2.pop_front();
    }
    return true;
}

// -------------------------------------------------------------------------------------

CachedImage::CachedImage(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate)
    : QObject(), CachedObject(url, Image, _cachePolicy, _expireDate)
#if APPLE_CHANGES
    , m_dataSize(0)
#endif
{
#if !APPLE_CHANGES
    static const QString &acceptHeader = KGlobal::staticQString( buildAcceptHeader() );
#endif

    m = 0;
    p = 0;
    pixPart = 0;
    bg = 0;
    bgColor = qRgba( 0, 0, 0, 0xFF );
#if !APPLE_CHANGES
    typeChecked = false;
#endif
    isFullyTransparent = false;
    errorOccured = false;
    monochrome = false;
    formatType = 0;
    m_status = Unknown;
    imgSource = 0;
    m_loading = true;
#if !APPLE_CHANGES
    setAccept( acceptHeader );
#endif
    m_showAnimations = dl->showAnimations();
}

CachedImage::~CachedImage()
{
    clear();
}

void CachedImage::ref( CachedObjectClient *c )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << " CachedImage::ref(" << c << ") " << endl;
#endif

    CachedObject::ref(c);

    if( m ) {
        m->unpause();
        if( m->finished() || m_clients.count() == 1 )
            m->restart();
    }

    // for mouseovers, dynamic changes
    if ( m_status >= Persistent && !valid_rect().isNull() )
        c->setPixmap( pixmap(), valid_rect(), this);

    if(!m_loading) c->notifyFinished(this);
}

void CachedImage::deref( CachedObjectClient *c )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << " CachedImage::deref(" << c << ") " << endl;
#endif
    Cache::flush();
    CachedObject::deref(c);
    if(m && m_clients.isEmpty() && m->running())
        m->pause();
    if ( canDelete() && m_free )
        delete this;
}

#define BGMINWIDTH      32
#define BGMINHEIGHT     32

const QPixmap &CachedImage::tiled_pixmap(const QColor& newc)
{
#if APPLE_CHANGES
    return pixmap();
#else
    static QRgb bgTransparant = qRgba( 0, 0, 0, 0xFF );
    if ( (bgColor != bgTransparant) && (bgColor != newc.rgb()) ) {
        delete bg; bg = 0;
    }

    if (bg)
        return *bg;

    const QPixmap &r = pixmap();

    if (r.isNull()) return r;

    // no error indication for background images
    if(errorOccured) return *Cache::nullPixmap;

    bool isvalid = newc.isValid();
    QSize s(pixmap_size());
    int w = r.width();
    int h = r.height();
    if ( w*h < 8192 )
    {
        if ( r.width() < BGMINWIDTH )
            w = ((BGMINWIDTH  / s.width())+1) * s.width();
        if ( r.height() < BGMINHEIGHT )
            h = ((BGMINHEIGHT / s.height())+1) * s.height();
    }
    if ( (w != r.width()) || (h != r.height()) || (isvalid && r.mask()))
    {
        QPixmap pix = r;
        if ( w != r.width() || (isvalid && pix.mask()))
        {
            bg = new QPixmap(w, r.height());
            QPainter p(bg);
            if(isvalid) p.fillRect(0, 0, w, r.height(), newc);
            p.drawTiledPixmap(0, 0, w, r.height(), pix);
            if(!isvalid && pix.mask())
            {
                // unfortunately our anti-transparency trick doesn't work here
                // we need to create a mask.
                QBitmap newmask(w, r.height());
                QPainter pm(&newmask);
                pm.drawTiledPixmap(0, 0, w, r.height(), *pix.mask());
                bg->setMask(newmask);
                bgColor = bgTransparant;
            }
            else
                bgColor= newc.rgb();
            pix = *bg;
        }
        if ( h != r.height() )
        {
            delete bg;
            bg = new QPixmap(w, h);
            QPainter p(bg);
            if(isvalid) p.fillRect(0, 0, w, h, newc);
            p.drawTiledPixmap(0, 0, w, h, pix);
            if(!isvalid && pix.mask())
            {
                // unfortunately our anti-transparency trick doesn't work here
                // we need to create a mask.
                QBitmap newmask(w, h);
                QPainter pm(&newmask);
                pm.drawTiledPixmap(0, 0, w, h, *pix.mask());
                bg->setMask(newmask);
                bgColor = bgTransparant;
            }
            else
                bgColor= newc.rgb();
        }
        return *bg;
    }

    return r;
#endif
}

const QPixmap &CachedImage::pixmap( ) const
{
    if(errorOccured)
        return *Cache::brokenPixmap;

#if APPLE_CHANGES
    if (p)
        return *p;
#else
    if(m)
    {
        if(m->framePixmap().size() != m->getValidRect().size() && m->getValidRect().size().isValid())
        {
            // pixmap is not yet completely loaded, so we
            // return a clipped version. asserting here
            // that the valid rect is always from 0/0 to fullwidth/ someheight
            if(!pixPart) pixPart = new QPixmap(m->getValidRect().size());

            (*pixPart) = m->framePixmap();
            pixPart->resize(m->getValidRect().size());
            return *pixPart;
        }
        else
            return m->framePixmap();
    }
    else if(p)
        return *p;
#endif // APPLE_CHANGES

    return *Cache::nullPixmap;
}


QSize CachedImage::pixmap_size() const
{
    return (m ? m->framePixmap().size() : ( p ? p->size() : QSize()));
}


QRect CachedImage::valid_rect() const
{
    return m ? m->getValidRect() : ( p ? p->rect() : QRect());
}


void CachedImage::do_notify(const QPixmap& p, const QRect& r)
{
    QPtrList<CachedObjectClient> clients(m_clients);
    CachedObjectClient *c;
    for ( c = clients.first(); c != 0; c = clients.next() )
        c->setPixmap( p, r, this);
}

#if !APPLE_CHANGES

void CachedImage::movieUpdated( const QRect& r )
{
#ifdef CACHE_DEBUG
    qDebug("movie updated %d/%d/%d/%d, pixmap size %d/%d", r.x(), r.y(), r.right(), r.bottom(),
           m->framePixmap().size().width(), m->framePixmap().size().height());
#endif

    do_notify(m->framePixmap(), r);
}

void CachedImage::movieStatus(int status)
{
#ifdef CACHE_DEBUG
    qDebug("movieStatus(%d)", status);
#endif

    // ### the html image objects are supposed to send the load event after every frame (according to
    // netscape). We have a problem though where an image is present, and js code creates a new Image object,
    // which uses the same CachedImage, the one in the document is not supposed to be notified

    // just another Qt 2.2.0 bug. we cannot call
    // QMovie::frameImage if we're after QMovie::EndOfMovie
    if(status == QMovie::EndOfFrame)
    {
        const QImage& im = m->frameImage();
        monochrome = ( ( im.depth() <= 8 ) && ( im.numColors() - int( im.hasAlphaBuffer() ) <= 2 ) );
        for (int i = 0; monochrome && i < im.numColors(); ++i)
            if (im.colorTable()[i] != qRgb(0xff, 0xff, 0xff) &&
                im.colorTable()[i] != qRgb(0x00, 0x00, 0x00))
                monochrome = false;
        if((im.width() < 5 || im.height() < 5) && im.hasAlphaBuffer()) // only evaluate for small images
        {
            QImage am = im.createAlphaMask();
            if(am.depth() == 1)
            {
                bool solid = false;
                for(int y = 0; y < am.height(); y++)
                    for(int x = 0; x < am.width(); x++)
                        if(am.pixelIndex(x, y)) {
                            solid = true;
                            break;
                        }
                isFullyTransparent = (!solid);
            }
        }

        // we have to delete our tiled bg variant here
        // because the frame has changed (in order to keep it in sync)
        delete bg;
        bg = 0;
    }


    if((status == QMovie::EndOfMovie && (!m || m->frameNumber() <= 1)) ||
       ((status == QMovie::EndOfLoop) && (m_showAnimations == KHTMLSettings::KAnimationLoopOnce)) ||
       ((status == QMovie::EndOfFrame) && (m_showAnimations == KHTMLSettings::KAnimationDisabled))
      )
    {
        if(imgSource)
        {
            setShowAnimations( KHTMLSettings::KAnimationDisabled );

            // monochrome alphamasked images are usually about 10000 times
            // faster to draw, so this is worth the hack
            if (p && monochrome && p->depth() > 1 )
            {
                QPixmap* pix = new QPixmap;
                pix->convertFromImage( p->convertToImage().convertDepth( 1 ), MonoOnly|AvoidDither );
                if ( p->mask() )
                    pix->setMask( *p->mask() );
                delete p;
                p = pix;
                monochrome = false;
            }
        }

        QPtrList<CachedObjectClient> clients(m_clients);
	CachedObjectClient *c;
        for ( c = clients.first(); c != 0; c = clients.next() )
            c->notifyFinished(this);
    }

    if((status == QMovie::EndOfFrame) || (status == QMovie::EndOfMovie))
    {
#ifdef CACHE_DEBUG
        QRect r(valid_rect());
        qDebug("movie Status frame update %d/%d/%d/%d, pixmap size %d/%d", r.x(), r.y(), r.right(), r.bottom(),
               pixmap().size().width(), pixmap().size().height());
#endif
            do_notify(pixmap(), valid_rect());
    }
}

void CachedImage::movieResize(const QSize& /*s*/)
{
//    do_notify(m->framePixmap(), QRect());
}

#endif // APPLE_CHANGES

void CachedImage::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    m_showAnimations = showAnimations;
#if !APPLE_CHANGES
    if ( (m_showAnimations == KHTMLSettings::KAnimationDisabled) && imgSource ) {
        imgSource->cleanBuffer();
        delete p;
        p = new QPixmap(m->framePixmap());

        m->disconnectUpdate( this, SLOT( movieUpdated( const QRect &) ));
        m->disconnectStatus( this, SLOT( movieStatus( int ) ));
        m->disconnectResize( this, SLOT( movieResize( const QSize& ) ) );
        QTimer::singleShot(0, this, SLOT( deleteMovie()));
        imgSource = 0;
    }
#endif
}

#if !APPLE_CHANGES

void CachedImage::deleteMovie()
{
    delete m; m = 0;
}

#endif // APPLE_CHANGES

void CachedImage::clear()
{
    delete m;   m = 0;
    delete p;   p = 0;
    delete bg;  bg = 0;
    bgColor = qRgba( 0, 0, 0, 0xff );
    delete pixPart; pixPart = 0;

    formatType = 0;

#if !APPLE_CHANGES
    typeChecked = false;
#endif
    setSize(0);

    // No need to delete imageSource - QMovie does it for us
    imgSource = 0;
}

void CachedImage::data ( QBuffer &_buffer, bool eof )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << "in CachedImage::data(buffersize " << _buffer.buffer().size() <<", eof=" << eof << endl;
#endif

#if !APPLE_CHANGES
    if ( !typeChecked )
    {
        formatType = QImageDecoder::formatName( (const uchar*)_buffer.buffer().data(), _buffer.size());

        typeChecked = true;

        if ( formatType )  // movie format exists
        {
            imgSource = new ImageSource( _buffer.buffer());
            m = new QMovie( imgSource, 8192 );
            m->connectUpdate( this, SLOT( movieUpdated( const QRect &) ));
            m->connectStatus( this, SLOT( movieStatus(int)));
            m->connectResize( this, SLOT( movieResize( const QSize& ) ) );
        }
    }

    if ( imgSource )
    {
        imgSource->setEOF(eof);
        imgSource->maybeReady();
    }

    if(eof)
    {
        // QMovie currently doesn't support all kinds of image formats
        // so we need to use a QPixmap here when we finished loading the complete
        // picture and display it then all at once.
        if(typeChecked && !formatType)
        {
#ifdef CACHE_DEBUG
            kdDebug(6060) << "CachedImage::data(): reloading as pixmap:" << endl;
#endif
            p = new QPixmap( _buffer.buffer() );
            // set size of image.
#ifdef CACHE_DEBUG
            kdDebug(6060) << "CachedImage::data(): image is null: " << p->isNull() << endl;
#endif
                if(p->isNull())
                {
                    errorOccured = true;
                    do_notify(pixmap(), QRect(0, 0, 16, 16)); // load "broken image" icon
                }
                else
                    do_notify(*p, p->rect());
        }

        QSize s = pixmap_size();
        setSize(s.width() * s.height() * 2);
    }
#else // APPLE_CHANGES
    bool canDraw = false;
    
    m_dataSize = _buffer.size();
        
    // If we're at eof and don't have a pixmap yet, the data
    // must have arrived in one chunk.  This avoids the attempt
    // to perform incremental decoding.
    if (eof && !p) {
        p = new QPixmap(_buffer.buffer());
        canDraw = true;
    } else {
        // Always attempt to load the image incrementally.
        // If the AppKit is unable to decode incrementally this pixmap
        // will not be renderable until all the data has been received.
        if (!p)
            p = new QPixmap;
        canDraw = p->receivedData(_buffer.buffer(), eof);
    }
    
    if (canDraw || eof) {
        if (p->isNull()) {
            errorOccured = true;
            do_notify(pixmap(), QRect(0, 0, 16, 16)); // load "broken image" icon
        } else {
            // May schedule a redraw.
            do_notify(*p, p->rect());
        }
        QSize s = pixmap_size();
        setSize(s.width() * s.height() * 2);
    }
    if (eof) {
	m_loading = false;
	checkNotify();
    }
#endif // APPLE_CHANGES
}

void CachedImage::error( int /*err*/, const char */*text*/ )
{
#ifdef CACHE_DEBUG
    kdDebug(6060) << "CahcedImage::error" << endl;
#endif

    clear();
#if !APPLE_CHANGES
    typeChecked = true;
#endif
    errorOccured = true;
    do_notify(pixmap(), QRect(0, 0, 16, 16));
#if APPLE_CHANGES
    m_loading = false;
    checkNotify();
#endif
}

void CachedImage::checkNotify()
{
    if(m_loading) return;

    QPtrList<CachedObjectClient> clients(m_clients);
    CachedObjectClient *c;
    for ( c = clients.first(); c != 0; c = clients.next() )
        c->notifyFinished(this);
}


// ------------------------------------------------------------------------------------------

Request::Request(DocLoader* dl, CachedObject *_object, bool _incremental)
{
    object = _object;
    object->setRequest(this);
    incremental = _incremental;
    m_docLoader = dl;
}

Request::~Request()
{
    object->setRequest(0);
}

// ------------------------------------------------------------------------------------------

DocLoader::DocLoader(KHTMLPart* part, DocumentImpl* doc)
{
    m_cachePolicy = KIO::CC_Verify;
    m_expireDate = 0;
    m_bautoloadImages = true;
    m_showAnimations = KHTMLSettings::KAnimationEnabled;
    m_part = part;
    m_doc = doc;

#if APPLE_CHANGES
    Cache::init();
#endif
    Cache::docloader->append( this );
}

DocLoader::~DocLoader()
{
    Cache::docloader->remove( this );
}

void DocLoader::setExpireDate(time_t _expireDate)
{
    m_expireDate = _expireDate;
}

bool DocLoader::needReload(const KURL &fullURL)
{
    bool reload = false;
    if (m_cachePolicy == KIO::CC_Verify)
    {
       if (!m_reloadedURLs.contains(fullURL.url()))
       {
          CachedObject *existing = Cache::cache->find(fullURL.url());
          if (existing && existing->isExpired())
          {
             Cache::removeCacheEntry(existing);
             m_reloadedURLs.append(fullURL.url());
             reload = true;
          }
       }
    }
    else if ((m_cachePolicy == KIO::CC_Reload) || (m_cachePolicy == KIO::CC_Refresh))
    {
       if (!m_reloadedURLs.contains(fullURL.url()))
       {
          CachedObject *existing = Cache::cache->find(fullURL.url());
          if (existing)
          {
             Cache::removeCacheEntry(existing);
          }
          m_reloadedURLs.append(fullURL.url());
          reload = true;
       }
    }
    return reload;
}

CachedImage *DocLoader::requestImage( const DOM::DOMString &url)
{
    KURL fullURL = m_doc->completeURL( url.string() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

#if APPLE_CHANGES
    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }
#endif

    bool reload = needReload(fullURL);

#if APPLE_CHANGES
    CachedImage *cachedObject = Cache::requestImage(this, url, reload, m_expireDate);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
#else
    return Cache::requestImage(this, url, reload, m_expireDate);
#endif
}

CachedCSSStyleSheet *DocLoader::requestStyleSheet( const DOM::DOMString &url, const QString& charset)
{
    KURL fullURL = m_doc->completeURL( url.string() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

#if APPLE_CHANGES
    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }
#endif

    bool reload = needReload(fullURL);

#if APPLE_CHANGES
    CachedCSSStyleSheet *cachedObject = Cache::requestStyleSheet(this, url, reload, m_expireDate, charset);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
#else
    return Cache::requestStyleSheet(this, url, reload, m_expireDate, charset);
#endif
}

CachedScript *DocLoader::requestScript( const DOM::DOMString &url, const QString& charset)
{
    KURL fullURL = m_doc->completeURL( url.string() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

#if APPLE_CHANGES
    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }
#endif

    bool reload = needReload(fullURL);

#if APPLE_CHANGES
    CachedScript *cachedObject = Cache::requestScript(this, url, reload, m_expireDate, charset);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
#else
    return Cache::requestScript(this, url, reload, m_expireDate, charset);
#endif
}

void DocLoader::setAutoloadImages( bool enable )
{
    if ( enable == m_bautoloadImages )
        return;

    m_bautoloadImages = enable;

    if ( !m_bautoloadImages ) return;

    for ( const CachedObject* co=m_docObjects.first(); co; co=m_docObjects.next() )
        if ( co->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>( co ) );

            CachedObject::Status status = img->status();
            if ( status != CachedObject::Unknown )
                continue;

            Cache::loader()->load(this, img, true);
        }
}

void DocLoader::setCachePolicy( KIO::CacheControl cachePolicy )
{
    m_cachePolicy = cachePolicy;
}

void DocLoader::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    if ( showAnimations == m_showAnimations ) return;
    m_showAnimations = showAnimations;

    const CachedObject* co;
    for ( co=m_docObjects.first(); co; co=m_docObjects.next() )
        if ( co->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>( co ) );

            img->setShowAnimations( showAnimations );
        }
}

void DocLoader::removeCachedObject( CachedObject* o ) const
{
    m_docObjects.removeRef( o );
}

// ------------------------------------------------------------------------------------------

Loader::Loader() : QObject()
{
    m_requestsPending.setAutoDelete( true );
    m_requestsLoading.setAutoDelete( true );
#if APPLE_CHANGES
    kwq = new KWQLoader(this);
#endif
}

Loader::~Loader()
{
#if APPLE_CHANGES
    delete kwq;
#endif
}

void Loader::load(DocLoader* dl, CachedObject *object, bool incremental)
{
    Request *req = new Request(dl, object, incremental);
    m_requestsPending.append(req);

    emit requestStarted( req->m_docLoader, req->object );

    servePendingRequests();
}

void Loader::servePendingRequests()
{
  if ( m_requestsPending.count() == 0 )
      return;

  // get the first pending request
  Request *req = m_requestsPending.take(0);

#ifdef CACHE_DEBUG
  kdDebug( 6060 ) << "starting Loader url=" << req->object->url().string() << endl;
#endif

  KURL u(req->object->url().string());
  KIO::TransferJob* job = KIO::get( u, false, false /*no GUI*/);

  job->addMetaData("cache", getCacheControlString(req->object->cachePolicy()));
  if (!req->object->accept().isEmpty())
      job->addMetaData("accept", req->object->accept());
  if ( req->m_docLoader )  {
      KURL r = req->m_docLoader->doc()->URL();
      if ( r.protocol().startsWith( "http" ) && r.path().isEmpty() )
          r.setPath( "/" );

      job->addMetaData("referrer", r.url());
      QString domain = r.host();
      if (req->m_docLoader->doc()->isHTMLDocument())
         domain = static_cast<HTMLDocumentImpl*>(req->m_docLoader->doc())->domain().string();
      if (crossDomain(u.host(), domain))
         job->addMetaData("cross-domain", "true");
  }

  connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotFinished( KIO::Job * ) ) );
  connect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
           SLOT( slotData( KIO::Job*, const QByteArray &)));

#if APPLE_CHANGES
  if (KWQServeRequest(this, req, job))
      m_requestsLoading.insert(job, req);
#else
  if ( req->object->schedule() )
      KIO::Scheduler::scheduleJob( job );

  m_requestsLoading.insert(job, req);
#endif // APPLE_CHANGES
}

void Loader::slotFinished( KIO::Job* job )
{
  Request *r = m_requestsLoading.take( job );
  KIO::TransferJob* j = static_cast<KIO::TransferJob*>(job);

  if ( !r )
    return;

  if (j->error() || j->isErrorPage())
  {
      kdDebug(6060) << "Loader::slotFinished, with error. job->error()= " << j->error() << " job->isErrorPage()=" << j->isErrorPage() << endl;
      r->object->error( job->error(), job->errorText().ascii() );
      emit requestFailed( r->m_docLoader, r->object );
  }
  else
  {
      r->object->data(r->m_buffer, true);
      emit requestDone( r->m_docLoader, r->object );
      time_t expireDate = j->queryMetaData("expire-date").toLong();
#if !APPLE_CHANGES
kdDebug(6060) << "Loader::slotFinished, url = " << j->url().url() << " expires " << ctime(&expireDate) << endl;
#endif
      r->object->setExpireDate(expireDate, false);
  }

#if APPLE_CHANGES
  // We don't want to ever finish the load in the case of an error because we don't want them cached.
  if (!j->error())
#endif
  r->object->finish();

#ifdef CACHE_DEBUG
  kdDebug( 6060 ) << "Loader:: JOB FINISHED " << r->object << ": " << r->object->url().string() << endl;
#endif

  delete r;
  servePendingRequests();
}

#if APPLE_CHANGES
void Loader::receivedResponse(KIO::Job* job, void *response)
{
    Request *r = m_requestsLoading[job];
    ASSERT(r);
    ASSERT(response);
    r->object->setResponse(response);
}
#endif

#if APPLE_CHANGES
void Loader::slotData( KIO::Job*job, const char *data, int size )
#else
void Loader::slotData( KIO::Job*job, const QByteArray &data )
#endif
{
    Request *r = m_requestsLoading[job];
    if(!r) {
        kdDebug( 6060 ) << "got data for unknown request!" << endl;
        return;
    }

    if ( !r->m_buffer.isOpen() )
        r->m_buffer.open( IO_WriteOnly );

#if APPLE_CHANGES
    r->m_buffer.writeBlock( data, size );
#else
    r->m_buffer.writeBlock( data.data(), data.size() );
#endif

    if(r->incremental)
        r->object->data( r->m_buffer, false );
}

int Loader::numRequests( DocLoader* dl ) const
{
    int res = 0;

    QPtrListIterator<Request> pIt( m_requestsPending );
    for (; pIt.current(); ++pIt )
        if ( pIt.current()->m_docLoader == dl )
            res++;

    QPtrDictIterator<Request> lIt( m_requestsLoading );
    for (; lIt.current(); ++lIt )
        if ( lIt.current()->m_docLoader == dl )
            res++;

    return res;
}

void Loader::cancelRequests( DocLoader* dl )
{
    //kdDebug( 6060 ) << "void Loader::cancelRequests()" << endl;
    //kdDebug( 6060 ) << "got " << m_requestsPending.count() << " pending requests" << endl;
    QPtrListIterator<Request> pIt( m_requestsPending );
    while ( pIt.current() )
    {
        if ( pIt.current()->m_docLoader == dl )
        {
            kdDebug( 6060 ) << "cancelling pending request for " << pIt.current()->object->url().string() << endl;
            //emit requestFailed( dl, pIt.current()->object );
            Cache::removeCacheEntry( pIt.current()->object );
            m_requestsPending.remove( pIt );
        }
        else
            ++pIt;
    }

    //kdDebug( 6060 ) << "got " << m_requestsLoading.count() << "loading requests" << endl;

    QPtrDictIterator<Request> lIt( m_requestsLoading );
    while ( lIt.current() )
    {
        if ( lIt.current()->m_docLoader == dl )
        {
            //kdDebug( 6060 ) << "cancelling loading request for " << lIt.current()->object->url().string() << endl;
            KIO::Job *job = static_cast<KIO::Job *>( lIt.currentKey() );
            Cache::removeCacheEntry( lIt.current()->object );
            m_requestsLoading.remove( lIt.currentKey() );
            job->kill();
            //emit requestFailed( dl, pIt.current()->object );
        }
        else
            ++lIt;
    }
}

KIO::Job *Loader::jobForRequest( const DOM::DOMString &url ) const
{
    QPtrDictIterator<Request> it( m_requestsLoading );

    for (; it.current(); ++it )
    {
        CachedObject *obj = it.current()->object;

        if ( obj && obj->url() == url )
            return static_cast<KIO::Job *>( it.currentKey() );
    }

    return 0;
}

// ----------------------------------------------------------------------------


LRUList::LRUList()
:m_head(0), m_tail(0) 
{}

LRUList::~LRUList()
{}

QDict<CachedObject> *Cache::cache = 0;
QPtrList<DocLoader>* Cache::docloader = 0;
Loader *Cache::m_loader = 0;

int Cache::maxSize = DEFCACHESIZE;
int Cache::flushCount = 0;

QPixmap *Cache::nullPixmap = 0;
QPixmap *Cache::brokenPixmap = 0;

CachedObject *Cache::m_headOfUncacheableList = 0;
int Cache::m_totalSizeOfLRULists = 0;
int Cache::m_countOfLRUAndUncacheableLists;
LRUList Cache::m_LRULists[MAX_LRU_LISTS];

void Cache::init()
{
    if ( !cache )
        cache = new QDict<CachedObject>(401, true);

    if ( !docloader )
        docloader = new QPtrList<DocLoader>;

    if ( !nullPixmap )
        nullPixmap = new QPixmap;

    if ( !brokenPixmap )
#if APPLE_CHANGES
        brokenPixmap = new QPixmap;
#else
//        brokenPixmap = new QPixmap(KHTMLFactory::instance()->iconLoader()->loadIcon("file_broken", KIcon::FileSystem, 16, KIcon::DisabledState));
        brokenPixmap = new QPixmap(KHTMLFactory::instance()->iconLoader()->loadIcon("file_broken", KIcon::Desktop, 16, KIcon::DisabledState));
#endif

    if ( !m_loader )
        m_loader = new Loader();
}

void Cache::clear()
{
    if ( !cache ) return;
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "Cache: CLEAR!" << endl;
    statistics();
#endif
    cache->setAutoDelete( true );
    delete cache; cache = 0;
    delete nullPixmap; nullPixmap = 0;
    delete brokenPixmap; brokenPixmap = 0;
    delete m_loader;   m_loader = 0;
    delete docloader; docloader = 0;
}

CachedImage *Cache::requestImage( DocLoader* dl, const DOMString & url, bool reload, time_t _expireDate )
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if ( dl )
    {
        kurl = dl->m_doc->completeURL( url.string() );
        cachePolicy = dl->cachePolicy();
    }
    else
    {
        kurl = url.string();
        cachePolicy = KIO::CC_Verify;
    }

#if APPLE_CHANGES
    // Checking if the URL is malformed is lots of extra work for little benefit.
#else
    if( kurl.isMalformed() )
    {
#ifdef CACHE_DEBUG
      kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
#endif
      return 0;
    }
#endif

    CachedObject *o = 0;
    if (!reload)
        o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedImage *im = new CachedImage(dl, kurl.url(), cachePolicy, _expireDate);
        if ( dl && dl->autoloadImages() ) Cache::loader()->load(dl, im, true);
#if APPLE_CHANGES
        if (cacheDisabled)
            im->setFree(true);
        else {
#endif
        cache->insert( kurl.url(), im );
        moveToHeadOfLRUList(im);
#if APPLE_CHANGES
        }
#endif
        o = im;
    }

    o->setExpireDate(_expireDate, true);

    if(!o->type() == CachedObject::Image)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestImage url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << ", status " << o->status() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
#if APPLE_CHANGES
        if (!cacheDisabled)
#endif
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedImage *>(o);
}

CachedCSSStyleSheet *Cache::requestStyleSheet( DocLoader* dl, const DOMString & url, bool reload, time_t _expireDate, const QString& charset)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if ( dl )
    {
        kurl = dl->m_doc->completeURL( url.string() );
        cachePolicy = dl->cachePolicy();
    }
    else
    {
        kurl = url.string();
        cachePolicy = KIO::CC_Verify;
    }

#if APPLE_CHANGES
    // Checking if the URL is malformed is lots of extra work for little benefit.
#else
    if( kurl.isMalformed() )
    {
      kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
      return 0;
    }
#endif

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedCSSStyleSheet *sheet = new CachedCSSStyleSheet(dl, kurl.url(), cachePolicy, _expireDate, charset);
#if APPLE_CHANGES
        if (cacheDisabled)
            sheet->setFree(true);
        else {
#endif
        cache->insert( kurl.url(), sheet );
        moveToHeadOfLRUList(sheet);
#if APPLE_CHANGES
        }
#endif
        o = sheet;
    }

    o->setExpireDate(_expireDate, true);

    if(!o->type() == CachedObject::CSSStyleSheet)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestStyleSheet url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
#if APPLE_CHANGES
        if (!cacheDisabled)
#endif
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedCSSStyleSheet *>(o);
}

void Cache::preloadStyleSheet( const QString &url, const QString &stylesheet_data)
{
    CachedObject *o = cache->find(url);
    if(o)
        removeCacheEntry(o);

    CachedCSSStyleSheet *stylesheet = new CachedCSSStyleSheet(url, stylesheet_data);
    cache->insert( url, stylesheet );
}

CachedScript *Cache::requestScript( DocLoader* dl, const DOM::DOMString &url, bool reload, time_t _expireDate, const QString& charset)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if ( dl )
    {
        kurl = dl->m_doc->completeURL( url.string() );
        cachePolicy = dl->cachePolicy();
    }
    else
    {
        kurl = url.string();
        cachePolicy = KIO::CC_Verify;
    }

#if APPLE_CHANGES
    // Checking if the URL is malformed is lots of extra work for little benefit.
#else
    if( kurl.isMalformed() )
    {
      kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
      return 0;
    }
#endif

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedScript *script = new CachedScript(dl, kurl.url(), cachePolicy, _expireDate, charset);
#if APPLE_CHANGES
        if (cacheDisabled)
            script->setFree(true);
        else {
#endif
        cache->insert( kurl.url(), script );
        moveToHeadOfLRUList(script);
#if APPLE_CHANGES
        }
#endif
        o = script;
    }

    o->setExpireDate(_expireDate, true);

    if(!(o->type() == CachedObject::Script))
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestScript url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
#if APPLE_CHANGES
        if (!cacheDisabled)
#endif
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedScript *>(o);
}

void Cache::preloadScript( const QString &url, const QString &script_data)
{
    CachedObject *o = cache->find(url);
    if(o)
        removeCacheEntry(o);

    CachedScript *script = new CachedScript(url, script_data);
    cache->insert( url, script );
}

void Cache::flush(bool force)
{
    if (force)
       flushCount = 0;
    // Don't flush for every image.
    if (m_countOfLRUAndUncacheableLists < flushCount)
       return;

    init();

#ifdef CACHE_DEBUG
    statistics();
    kdDebug( 6060 ) << "Cache: flush()" << endl;
#endif

    while (m_headOfUncacheableList)
        removeCacheEntry(m_headOfUncacheableList);
    
    for (int i = MAX_LRU_LISTS-1; i>=0; i--) {
        if (m_totalSizeOfLRULists <= maxSize)
            break;
            
        while (m_totalSizeOfLRULists > maxSize && m_LRULists[i].m_tail) {
            removeCacheEntry(m_LRULists[i].m_tail);
        }
    }

    flushCount = m_countOfLRUAndUncacheableLists+10; // Flush again when the cache has grown.
#ifdef CACHE_DEBUG
    //statistics();
#endif
}

#if 0

void Cache::checkLRUAndUncacheableListIntegrity()
{
    int count = 0;
    
    {
        int size = 0;
        CachedObject *prev = 0;
        for (CachedObject *o = m_headOfLRUList; o; o = o->m_nextInLRUList) {
            ASSERT(o->allowInLRUList());
            ASSERT(o->status() != CachedObject::Uncacheable);
            ASSERT(o->m_prevInLRUList == prev);
            size += o->size();
            prev = o;
            ++count;
        }
        ASSERT(m_tailOfLRUList == prev);
        ASSERT(m_totalSizeOfLRUList == size);
    }
    
    {
        CachedObject *prev = 0;
        for (CachedObject *o = m_headOfUncacheableList; o; o = o->m_nextInLRUList) {
            ASSERT(o->allowInLRUList());
            ASSERT(o->status() == CachedObject::Uncacheable);
            ASSERT(o->m_prevInLRUList == prev);
            prev = o;
            ++count;
        }
    }
    
    ASSERT(m_countOfLRUAndUncacheableLists == count);
}

#endif

void Cache::setSize( int bytes )
{
    maxSize = bytes;
    // may be we need to clear parts of the cache
    flushCount = 0;
    flush(true);
}

void Cache::statistics()
{
    CachedObject *o;
    // this function is for debugging purposes only
    init();

    int size = 0;
    int msize = 0;
    int movie = 0;
    int stylesheets = 0;
    QDictIterator<CachedObject> it(*cache);
    for(it.toFirst(); it.current(); ++it)
    {
        o = it.current();
        if(o->type() == CachedObject::Image)
        {
            CachedImage *im = static_cast<CachedImage *>(o);
            if(im->m != 0)
            {
                movie++;
                msize += im->size();
            }
        }
        else
        {
            if(o->type() == CachedObject::CSSStyleSheet)
                stylesheets++;

        }
        size += o->size();
    }
    size /= 1024;

    kdDebug( 6060 ) << "------------------------- image cache statistics -------------------" << endl;
    kdDebug( 6060 ) << "Number of items in cache: " << cache->count() << endl;
    kdDebug( 6060 ) << "Number of items in lru  : " << m_countOfLRUAndUncacheableLists << endl;
    kdDebug( 6060 ) << "Number of cached images: " << cache->count()-movie << endl;
    kdDebug( 6060 ) << "Number of cached movies: " << movie << endl;
    kdDebug( 6060 ) << "Number of cached stylesheets: " << stylesheets << endl;
    kdDebug( 6060 ) << "pixmaps:   allocated space approx. " << size << " kB" << endl;
    kdDebug( 6060 ) << "movies :   allocated space approx. " << msize/1024 << " kB" << endl;
    kdDebug( 6060 ) << "--------------------------------------------------------------------" << endl;
}

void Cache::removeCacheEntry( CachedObject *object )
{
  QString key = object->url().string();

  // this indicates the deref() method of CachedObject to delete itself when the reference counter
  // drops down to zero
  object->setFree( true );

  cache->remove( key );
  removeFromLRUList(object);

  const DocLoader* dl;
  for ( dl=docloader->first(); dl; dl=docloader->next() )
      dl->removeCachedObject( object );

  if ( object->canDelete() )
     delete object;
}

#define FAST_LOG2(_log2,_n)   \
      unsigned int j_ = (unsigned int)(_n);   \
      (_log2) = 0;                    \
      if ((j_) & ((j_)-1))            \
      (_log2) += 1;               \
      if ((j_) >> 16)                 \
      (_log2) += 16, (j_) >>= 16; \
      if ((j_) >> 8)                  \
      (_log2) += 8, (j_) >>= 8;   \
      if ((j_) >> 4)                  \
      (_log2) += 4, (j_) >>= 4;   \
      if ((j_) >> 2)                  \
      (_log2) += 2, (j_) >>= 2;   \
      if ((j_) >> 1)                  \
      (_log2) += 1;

int FastLog2(unsigned int i) {
    int log2;
    FAST_LOG2(log2,i);
    return log2;
}

LRUList* Cache::getLRUListFor(CachedObject* o)
{
    int accessCount = o->accessCount();
    int queueIndex;
    if (accessCount == 0) {
        queueIndex = 0;
    } else {
        int sizeLog = FastLog2(o->size());
        queueIndex = sizeLog/o->accessCount() - 1;
        if (queueIndex < 0)
            queueIndex = 0;
        if (queueIndex >= MAX_LRU_LISTS)
            queueIndex = MAX_LRU_LISTS-1;
    }
    return &m_LRULists[queueIndex];
}

void Cache::removeFromLRUList(CachedObject *object)
{
    CachedObject *next = object->m_nextInLRUList;
    CachedObject *prev = object->m_prevInLRUList;
    bool uncacheable = object->status() == CachedObject::Uncacheable;
    
    LRUList* list = uncacheable ? 0 : getLRUListFor(object);
    CachedObject *&head = uncacheable ? m_headOfUncacheableList : list->m_head;
    
    if (next == 0 && prev == 0 && head != object) {
        return;
    }
    
    object->m_nextInLRUList = 0;
    object->m_prevInLRUList = 0;
    
    if (next)
        next->m_prevInLRUList = prev;
    else if (!uncacheable && list->m_tail == object)
        list->m_tail = prev;

    if (prev)
        prev->m_nextInLRUList = next;
    else if (head == object)
        head = next;
    
    --m_countOfLRUAndUncacheableLists;
    
    if (!uncacheable)
        m_totalSizeOfLRULists -= object->size();
}

void Cache::moveToHeadOfLRUList(CachedObject *object)
{
    insertInLRUList(object);
}

void Cache::insertInLRUList(CachedObject *object)
{
    removeFromLRUList(object);
    
    if (!object->allowInLRUList())
        return;
    
    LRUList* list = getLRUListFor(object);
    
    bool uncacheable = object->status() == CachedObject::Uncacheable;
    CachedObject *&head = uncacheable ? m_headOfUncacheableList : list->m_head;

    object->m_nextInLRUList = head;
    if (head)
        head->m_prevInLRUList = object;
    head = object;
    
    if (object->m_nextInLRUList == 0 && !uncacheable)
        list->m_tail = object;
    
    ++m_countOfLRUAndUncacheableLists;
    
    if (!uncacheable)
        m_totalSizeOfLRULists += object->size();
}

bool Cache::adjustSize(CachedObject *object, int delta)
{
    if (object->status() == CachedObject::Uncacheable)
        return false;

    if (object->m_nextInLRUList == 0 && object->m_prevInLRUList == 0 &&
        getLRUListFor(object)->m_head != object)
        return false;
    
    m_totalSizeOfLRULists += delta;
    return delta != 0;
}

// --------------------------------------

void CachedObjectClient::setPixmap(const QPixmap &, const QRect&, CachedImage *) {}
void CachedObjectClient::setStyleSheet(const DOM::DOMString &/*url*/, const DOM::DOMString &/*sheet*/) {}
void CachedObjectClient::notifyFinished(CachedObject * /*finishedObj*/) {}


#include "loader.moc"


#if APPLE_CHANGES

Cache::Statistics Cache::getStatistics()
{
    Statistics stats;

    if (!cache)
        return stats;

    QDictIterator<CachedObject> i(*cache);
    for (i.toFirst(); i.current(); ++i) {
        CachedObject *o = i.current();
        switch (o->type()) {
            case CachedObject::Image:
                if (static_cast<CachedImage *>(o)->m) {
                    stats.movies.count++;
                    stats.movies.size += o->size();
                } else {
                    stats.images.count++;
                    stats.images.size += o->size();
                }
                break;

            case CachedObject::CSSStyleSheet:
                stats.styleSheets.count++;
                stats.styleSheets.size += o->size();
                break;

            case CachedObject::Script:
                stats.scripts.count++;
                stats.scripts.size += o->size();
                break;

            default:
                stats.other.count++;
                stats.other.size += o->size();
        }
    }
    
    return stats;
}

void Cache::flushAll()
{
    if (!cache)
        return;

    for (;;) {
        QDictIterator<CachedObject> i(*cache);
        CachedObject *o = i.toFirst();
        if (!o)
            break;
        removeCacheEntry(o);
    }
}

void Cache::setCacheDisabled(bool disabled)
{
    cacheDisabled = disabled;
    if (disabled)
        flushAll();
}

#endif // APPLE_CHANGES
