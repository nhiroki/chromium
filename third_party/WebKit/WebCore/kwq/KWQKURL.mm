/*
 * Copyright (C) 2001, 2002 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <kurl.h>
#include <kwqdebug.h>

#import <Foundation/NSURLPathUtilities.h>

class KURL::KWQKURLPrivate
{
public:
    KWQKURLPrivate(const QString &url);
    KWQKURLPrivate(const KWQKURLPrivate &other);
    ~KWQKURLPrivate();

    void init(const QString &);
    void makeRef();
    void decompose();
    void compose();
    
    CFURLRef urlRef;
    QString sURL;
    QString sProtocol;
    QString sHost;
    unsigned short int iPort;
    QString sPass;
    QString sUser;
    QString sRef;
    QString sQuery;
    QString sPath;
    QString escapedPath;
    bool addedSlash;

    int refCount;
};


KURL::KWQKURLPrivate::KWQKURLPrivate(const QString &url) :
    urlRef(NULL),
    iPort(0),
    addedSlash(false),
    refCount(0)
{
    init(url);
}

KURL::KWQKURLPrivate::KWQKURLPrivate(const KWQKURLPrivate &other) :
    urlRef(other.urlRef != NULL ? (CFURLRef)CFRetain(other.urlRef) : NULL),
    sURL(other.sURL),
    sProtocol(other.sProtocol),
    sHost(other.sHost),
    iPort(other.iPort),   
    sPass(other.sPass),   
    sUser(other.sUser),   
    sRef(other.sRef),  
    sQuery(other.sQuery),   
    sPath(other.sPath),   
    escapedPath(other.escapedPath),   
    addedSlash(other.addedSlash),
    refCount(0)
{
}

KURL::KWQKURLPrivate::~KWQKURLPrivate()
{
    if (urlRef != NULL) {
        CFRelease(urlRef);
    }
}

void KURL::KWQKURLPrivate::init(const QString &s)
{
    // Save original string
    sURL = s;

    decompose();
}

void KURL::KWQKURLPrivate::makeRef()
{
    // Not a path and no scheme, so bail out, because CFURL considers such
    // things valid URLs for some reason.
    if (!sURL.contains(':') && (sURL.length() == 0 || sURL[0] != '/') || sURL == "file:" || sURL == "file://") {
        urlRef = NULL;
        return;
    }

    // Create CFURLRef object
    if (sURL.length() > 0 && sURL[0] == '/') {
        sURL = (QString("file://")) + sURL;
    } else if (sURL.startsWith("file:/") && !sURL.startsWith("file://")) {
        sURL = (QString("file:///") + sURL.mid(6));
    }

    QString sURLMaybeAddSlash;
    int colonPos = sURL.find(':');
    int slashSlashPos = sURL.find("//", colonPos);
    if (slashSlashPos != -1 && sURL.find('/', slashSlashPos + 2) == -1) {
        sURLMaybeAddSlash = sURL + "/";
	addedSlash = true;
    } else {
        sURLMaybeAddSlash = sURL;
	addedSlash = false;
    }

    // Escape illegal but unambiguous characters that are actually
    // found on the web in URLs, like ' ' or '|'
    CFStringRef escaped = CFURLCreateStringByAddingPercentEscapes(NULL, sURLMaybeAddSlash.getCFMutableString(),
    								  CFSTR("%#"), NULL, kCFStringEncodingUTF8);

    urlRef = CFURLCreateWithString(NULL, escaped, NULL);

    CFRelease(escaped);
}

static inline QString CFStringToQString(CFStringRef cfs)
{
    QString qs;

    if (cfs != NULL) {
        qs = QString::fromCFString(cfs);
	CFRelease(cfs);
    } else {
        qs = QString();
    }

    return qs;
}

static inline QString escapeQString(QString str)
{
    return CFStringToQString(CFURLCreateStringByAddingPercentEscapes(NULL, str.getCFMutableString(), NULL, NULL, kCFStringEncodingUTF8));
}

static bool pathEndsWithSlash(QString sURL)
{
    int endOfPath = sURL.findRev('?', sURL.findRev('#'));
    if (endOfPath == -1) {
        endOfPath = sURL.length();
    }
    if (sURL[endOfPath-1] == '/') {
        return true;
    } else {
        return false;
    }
}


CFStringRef KWQCFURLCopyEscapedPath(CFURLRef anURL)
{
    NSRange path;
    CFStringRef urlString = CFURLGetString(anURL);

    _NSParseStringToGenericURLComponents((NSString *)urlString, NULL, NULL, NULL, NULL, NULL, &path, NULL, NULL, NULL);

    return CFStringCreateWithSubstring(NULL, urlString, CFRangeMake(path.location, path.length));
}

void KURL::KWQKURLPrivate::decompose()
{
    makeRef();

    if (urlRef == NULL) {
        // failed to parse
        return;
    }

    // decompose into parts

    sProtocol = CFStringToQString(CFURLCopyScheme(urlRef));

    QString hostName = CFStringToQString(CFURLCopyHostName(urlRef));
    if (sProtocol != "file") {
        sHost =  hostName;
    }

    SInt32 portNumber = CFURLGetPortNumber(urlRef);
    if (portNumber < 0) {
        iPort = 0;
    } else {
        iPort = portNumber;
    }

    sPass = CFStringToQString(CFURLCopyPassword(urlRef));
    sUser = CFStringToQString(CFURLCopyUserName(urlRef));
    sRef = CFStringToQString(CFURLCopyFragment(urlRef, NULL));

    sQuery = CFStringToQString(CFURLCopyQueryString(urlRef, NULL));
    if (!sQuery.isEmpty()) {
        sQuery = QString("?") + sQuery;
    }

    if (CFURLCanBeDecomposed(urlRef)) {
	if (addedSlash) {
	    sPath = "";
	    escapedPath = "";
	} else {
	    sPath = CFStringToQString(CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle));
	    escapedPath = CFStringToQString(KWQCFURLCopyEscapedPath(urlRef));
	}
	QString param = CFStringToQString(CFURLCopyParameterString(urlRef, CFSTR("")));
	if (!param.isEmpty()) {
	    sPath += ";" + param;
	    QString escapedParam = CFStringToQString(CFURLCopyParameterString(urlRef, NULL));

	    escapedPath += ";" + escapedParam;
	}

	if (pathEndsWithSlash(sURL) && sPath.right(1) != "/") {
	    sPath += "/";
	}
    } else {
        sPath = CFStringToQString(CFURLCopyResourceSpecifier(urlRef));
	escapedPath = sPath;
    }

    if (sProtocol == "file" && !hostName.isEmpty() && hostName != "localhost") {
        sPath = QString("//") + hostName + sPath;
	escapedPath = QString("//") + hostName + escapedPath;
    }

    // could lead to poor performance - perhaps compose manually in ::url?
    if (sURL != "file://localhost") {
	compose();
    }
}

void KURL::KWQKURLPrivate::compose()
{
    if (!sProtocol.isEmpty()) {
        QString result = escapeQString(sProtocol) + ":";

	if (!sHost.isEmpty()) {
	    result += "//";
	    if (!sUser.isEmpty()) {
	        result += escapeQString(sUser);
		if (!sPass.isEmpty()) {
		    result += ":" + escapeQString(sPass);
		}
		result += "@";
	    }
	    result += escapeQString(sHost);
	    if (iPort != 0) {
	        result += ":" + QString::number(iPort);
	    }
	}

	result += escapedPath + sQuery;
	
	if (!sRef.isEmpty()) {
	    result += "#" + sRef;
	}

	if (urlRef != NULL) {
	    CFRelease(urlRef);
	    urlRef = NULL;
	}

	sURL = result;

	makeRef();
    }
}

struct RelativeURLKey {
    CFStringRef base;
    CFStringRef relative;
};

static const void *RelativeURLKeyRetainCallBack(CFAllocatorRef allocator, const void *value)
{
    RelativeURLKey *key = (RelativeURLKey *)value;
    CFRetain(key->base);
    CFRetain(key->relative);
    return key;
}

static void RelativeURLKeyReleaseCallBack(CFAllocatorRef allocator, const void *value)
{
    RelativeURLKey *key = (RelativeURLKey *)value;
    CFRelease(key->base);
    CFRelease(key->relative);
    delete key;
}

static CFStringRef RelativeURLKeyCopyDescriptionCallBack(const void *value)
{
    return CFSTR("");
}

static unsigned char RelativeURLKeyEqualCallBack(const void *value1, const void *value2)
{
    RelativeURLKey *key1 = (RelativeURLKey *)value1;
    RelativeURLKey *key2 = (RelativeURLKey *)value2;
    
    return CFEqual(key1->base, key2->base) && CFEqual(key1->relative, key2->relative);
}

static CFHashCode RelativeURLKeyHashCallBack(const void *value)
{
    RelativeURLKey *key = (RelativeURLKey *)value;
    return CFHash(key->base) ^ CFHash(key->relative);
}

static const  CFDictionaryKeyCallBacks RelativeURLKeyCallBacks = {
    0,
    RelativeURLKeyRetainCallBack,
    RelativeURLKeyReleaseCallBack,
    RelativeURLKeyCopyDescriptionCallBack,
    RelativeURLKeyEqualCallBack,
    RelativeURLKeyHashCallBack,
};


static CFMutableDictionaryRef NormalizedURLCache = NULL;
static CFMutableDictionaryRef NormalizedRelativeURLCache = NULL;

void KURL::clearCaches()
{
    if (NormalizedURLCache != NULL) {
	CFDictionaryRemoveAllValues(NormalizedURLCache);
    }
    if (NormalizedRelativeURLCache != NULL) {
	CFDictionaryRemoveAllValues(NormalizedRelativeURLCache);
    }
}

QString KURL::normalizeURLString(const QString &s)
{
    CFMutableStringRef result = NULL;

    if (NormalizedURLCache == NULL) {
	NormalizedURLCache = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks); 
    }

    result = (CFMutableStringRef)CFDictionaryGetValue(NormalizedURLCache, s.getCFMutableString());

    if (result != NULL) {
	return QString::fromCFMutableString(result);
    } else {
	// normalize the URL string as KURL would:

	QString qurl = QString(s);

	// Special handling for paths
	if (!qurl.isEmpty() && qurl[0] == '/') {
	    qurl = QString("file:") + qurl;
	}

	if (d.isNull()) {
	    d = KWQRefPtr<KWQKURLPrivate>(new KWQKURLPrivate(qurl));
	}

	qurl = d->sURL;

	if (qurl.startsWith("file:///")) {
	    qurl = QString("file:/") + qurl.mid(8);
	} else if (qurl == "file://localhost") {
	    qurl = QString("file:");
	} else if (qurl.startsWith("file://localhost/")) {
	    qurl = QString("file:/") + qurl.mid(17);
	}

	CFDictionarySetValue(NormalizedURLCache, s.getCFMutableString(), qurl.getCFMutableString());

	return qurl;
    }
}

static CFStringRef copyAndReplaceAll(CFStringRef string, CFStringRef stringToFind, CFStringRef replacement)
{
    CFMutableStringRef copy = CFStringCreateMutableCopy(0, 0, string);
    while (true) {
        CFRange foundSubstring = CFStringFind(copy, stringToFind, 0);
        if (foundSubstring.location == kCFNotFound) {
            return copy;
        }
        CFStringReplace(copy, foundSubstring, replacement);
    }
}

static bool needToHideColons(CFStringRef string)
{
    // Do a few quick checks so we don't slow down the normal case too much.
    // These checks are taken straight out of the buggy code in CFURL.
    
    CFRange colon = CFStringFind(string, CFSTR(":"), 0);
    if (colon.location == kCFNotFound) {
        return false;
    }
    
    CFRange hash = CFStringFind(string, CFSTR("#"), kCFCompareBackwards);
    if (!(hash.location == kCFNotFound || hash.location > colon.location)) {
        return false;
    }
    
    // At this point, we have established the the CFURL code would treat this as
    // an absolute URL. Now check if there are any "?" or "/" characters before
    // the first colon; if there are, we need to "hide" the colons.
    CFRange beforeColon = CFRangeMake(0, colon.location - 1);
    return CFStringFindWithOptions(string, CFSTR("?"), beforeColon, 0, NULL)
        || CFStringFindWithOptions(string, CFSTR("/"), beforeColon, 0, NULL);
}

QString KURL::normalizeRelativeURLString(const KURL &base, const QString &relative)
{
    CFMutableStringRef result = NULL;
    RelativeURLKey key = { base.urlString.getCFMutableString(), relative.getCFMutableString() };

    if (NormalizedRelativeURLCache == NULL) {
	NormalizedRelativeURLCache = CFDictionaryCreateMutable(NULL, 0, &RelativeURLKeyCallBacks, &kCFTypeDictionaryValueCallBacks); 
    }

    result = (CFMutableStringRef)CFDictionaryGetValue(NormalizedRelativeURLCache, &key);

    if (result != NULL) {
	return QString::fromCFMutableString(result);
    } else {
	QString result;
	if (relative.isEmpty()) {
	    result = base.urlString;
	} else {
	    base.parse();

	    CFStringRef relativeURLString;
	    CFStringRef escapedString = CFURLCreateStringByAddingPercentEscapes(NULL, relative.getCFMutableString(),
										CFSTR("%#"), NULL, kCFStringEncodingUTF8);

            // Workaround for CFURL bug with colons, Radar 2891336.
            bool hideColons = needToHideColons(escapedString );
            if (hideColons) {
                relativeURLString = copyAndReplaceAll(escapedString, CFSTR(":"), CFSTR("INTRIGUE_COLON"));
		CFRelease(escapedString);
            } else {
		relativeURLString = escapedString;
	    }
            
	    CFURLRef relativeURL = CFURLCreateWithString(NULL, relativeURLString, base.d->urlRef);

	    CFRelease(relativeURLString);

	    if (relativeURL == NULL) {
		result = normalizeURLString(relative);
	    } else {
		CFURLRef absoluteURL = CFURLCopyAbsoluteURL(relativeURL);
                CFStringRef absoluteURLString = CFURLGetString(absoluteURL);
                
                if (hideColons) {
                    absoluteURLString = copyAndReplaceAll(absoluteURLString, CFSTR("INTRIGUE_COLON"), CFSTR(":"));
                }
                
		result = normalizeURLString(QString::fromCFString(absoluteURLString));
                
                if (hideColons) {
                    CFRelease(absoluteURLString);
                }
                
		CFRelease(relativeURL);
		CFRelease(absoluteURL);
	    }
	}
		
	CFDictionarySetValue(NormalizedRelativeURLCache, 
			     new RelativeURLKey(key), 
			     result.getCFMutableString());
	return result;
    }
}

// KURL

KURL::KURL() : 
    d(NULL),
    urlString(normalizeURLString(QString()))
{
}

KURL::KURL(const char *url, int encoding_hint) :
    d(NULL),
    urlString(normalizeURLString(url))
{
}

KURL::KURL(const QString &url, int encoding_hint) :
    d(NULL),
    urlString(normalizeURLString(url))
{
}

KURL::KURL(const KURL &base, const QString &relative) :
    d(NULL),
    urlString(normalizeRelativeURLString(base, relative))
{
}

KURL::KURL(const KURL &other) : 
    d(other.d),
    urlString(other.urlString)
{
}

KURL::~KURL()
{
}

bool KURL::isEmpty() const
{
    parse();
    return d->sURL.isEmpty();
}

bool KURL::isMalformed() const
{
    parse();
    return (d->urlRef == NULL);
}

bool KURL::hasPath() const
{
    parse();
    return !d->sPath.isEmpty();
}

QString KURL::url() const
{
    return urlString;
}

QString KURL::protocol() const
{
    parse();
    return d->sProtocol;
}

QString KURL::host() const
{
    parse();
    return d->sHost;
}

unsigned short int KURL::port() const
{
    parse();
    return d->iPort;
}

QString KURL::pass() const
{
    parse();
    return d->sPass;
}

QString KURL::user() const
{
    parse();
    return d->sUser;
}

QString KURL::ref() const
{
    parse();
    return d->sRef;
}

QString KURL::query() const
{
    parse();
    return d->sQuery;
}

QString KURL::path() const
{
    parse();
    return d->sPath;
}

void KURL::setProtocol(const QString &s)
{
    copyOnWrite();
    d->sProtocol = s;
    assemble();
}

void KURL::setHost(const QString &s)
{
    copyOnWrite();
    d->sHost = s;
    assemble();
}

void KURL::setPort(unsigned short i)
{
    copyOnWrite();
    d->iPort = i;
    assemble();
}

void KURL::setRef(const QString &s)
{
    copyOnWrite();
    d->sRef = s;
    assemble();
}

void KURL::setQuery(const QString &query, int encoding_hint)
{
    copyOnWrite();
    if (query.isEmpty() || query[0] == '?') {
	d->sQuery = query;
    } else {
	d->sQuery = "?" + query;
    }
    assemble();
}

void KURL::setPath(const QString &s)
{
    copyOnWrite();
    d->sPath = s;
    d->escapedPath = escapeQString(s);
    assemble();
}

QString KURL::prettyURL(int trailing) const
{
    parse();
    if (d->urlRef == NULL) {
        return d->sURL;
    }

    QString result =  d->sProtocol + ":";

    if (!d->sHost.isEmpty()) {
        result += "//";
	if (!d->sUser.isEmpty()) {
	    result += d->sUser;
	    result += "@";
	}
	result += d->sHost;
	if (d->iPort != 0) {
	    result += ":";
	    result += QString::number(d->iPort);
	}
    }

    QString path = d->sPath;

    if (trailing == 1) {
        if (path.right(1) != "/" && !path.isEmpty()) {
	    path += "/";
	}
    } else if (trailing == -1) {
        if (path.right(1) == "/" && path.length() > 1) {
	    path = path.left(path.length()-1);
	}
    }

    result += path;
    result += d->sQuery;

    if (!d->sRef.isEmpty()) {
        result += "#" + d->sRef;
    }

    return result;
}


void KURL::swap(KURL &other)
{
    KWQRefPtr<KWQKURLPrivate> tmpD = other.d;
    QString tmpString = other.urlString;
    
    other.d = d;
    d = tmpD;
    other.urlString = urlString;
    urlString = tmpString;
}   


KURL &KURL::operator=(const KURL &other)
{
    KURL(other).swap(*this);
    return *this;
}

QString KURL::decode_string(const QString &urlString)
{
    CFStringRef unescaped = CFURLCreateStringByReplacingPercentEscapes(NULL, urlString.getCFMutableString(), CFSTR(""));
    QString qUnescaped = QString::fromCFString(unescaped);
    CFRelease(unescaped);

    return qUnescaped;
}

void KURL::copyOnWrite()
{
    parse();
    if (d->refCount > 1) {
	d = KWQRefPtr<KWQKURLPrivate>(new KWQKURLPrivate(*d));
    }
}

void KURL::parse() const
{
    if (d.isNull()) {
	d = KWQRefPtr<KWQKURLPrivate>(new KWQKURLPrivate(urlString));
    }
}

void KURL::assemble()
{
    if (!d.isNull()) {
	d->compose();
	urlString = d->sURL;
    }
}

NSURL *KURL::getNSURL() const
{
    parse();
    return [[(NSURL *)d->urlRef retain] autorelease];
}

QString KURL::encodedHtmlRef() const
{
    _logNotYetImplemented();
    return 0;
}

QString KURL::htmlRef() const
{
    _logNotYetImplemented();
    return 0;
}
