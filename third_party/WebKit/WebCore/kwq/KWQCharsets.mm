/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#import "KWQCharsets.h"

struct CharsetEntry {
    const char *name;
    CFStringEncoding encoding;
};

/* The following autogenerated file includes the charset data. */
#import "KWQCharsetData.c"

static Boolean encodingNamesEqual(const void *value1, const void *value2);
static CFHashCode encodingNameHash(const void *value);

static CFDictionaryKeyCallBacks encodingNameKeyCallbacks = { 0, NULL, NULL, NULL, encodingNamesEqual, encodingNameHash };

static CFMutableDictionaryRef nameToEncoding = NULL;
static CFMutableDictionaryRef encodingToName = NULL;

static void buildDictionaries()
{
    nameToEncoding = CFDictionaryCreateMutable(NULL, 0, &encodingNameKeyCallbacks, NULL);
    encodingToName = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);

    for (int i = 0; table[i].name != NULL; i++) {
        CFDictionarySetValue(nameToEncoding, table[i].name, (void *)table[i].encoding);
        CFDictionarySetValue(encodingToName, (void *)table[i].encoding, table[i].name);
    }
}

CFStringEncoding KWQCFStringEncodingFromIANACharsetName(const char *name)
{
    if (nameToEncoding == NULL) {
        buildDictionaries();
    }

    const void *value;
    if (!CFDictionaryGetValueIfPresent(nameToEncoding, name, &value)) {
        return kCFStringEncodingInvalidId;
    }
    return (CFStringEncoding)value;
}

const char *KWQCFStringEncodingToIANACharsetName(CFStringEncoding encoding)
{
    if (encodingToName == NULL) {
        buildDictionaries();
    }
    
    const void *value;
    if (!CFDictionaryGetValueIfPresent(encodingToName, (void *)encoding, &value)) {
        return NULL;
    }
    return (const char *)value;
}

static Boolean encodingNamesEqual(const void *value1, const void *value2)
{
    const char *s1 = (const char *)value1;
    const char *s2 = (const char *)value2;
    
    while (1) {
        char c1;
        do {
            c1 = *s1++;
        } while (c1 && !isalnum(c1));
        char c2;
        do {
            c2 = *s2++;
        } while (c2 && !isalnum(c2));
        
        if (tolower(c1) != tolower(c2)) {
            return false;
        }
        
        if (!c1 || !c2) {
            return !c1 && !c2;
        }
    }
}

// Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
// or anything like that.
const unsigned PHI = 0x9e3779b9U;

// This hash algorithm comes from:
// http://burtleburtle.net/bob/hash/hashfaq.html
// http://burtleburtle.net/bob/hash/doobs.html
static CFHashCode encodingNameHash(const void *value)
{
    const char *s = (const char *)value;
    
    CFHashCode h = PHI;

    for (int i = 0; i != 16; ++i) {
        char c;
        do {
            c = *s++;
        } while (c && !isalnum(c));
        if (!c) {
            break;
        }
        h += c;
	h += (h << 10); 
	h ^= (h << 6); 
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
 
    if (h == 0) {
        h = 0x80000000;
    }

    return h;
}
