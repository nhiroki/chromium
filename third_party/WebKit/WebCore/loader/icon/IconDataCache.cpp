/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#include "IconDatabase.h"

#include "Logging.h"
#include "Image.h"
#include <limits.h>

using namespace WebCore;

IconDataCache::IconDataCache(const String& url)
    : m_iconURL(url)
    , m_stamp(0)
    , m_image(0)
    , m_dataSet(false)
{

}

IconDataCache::~IconDataCache()
{
    // Upon destruction of a IconDataCache, its image should no longer be in use anywhere
    delete m_image;
}

Image* IconDataCache::getImage(const IntSize& size)
{
    // FIXME rdar://4680377 - For size right now, we are returning our one and only image and the Bridge
    // is resizing it in place.  We need to actually store all the original representations here and return a native
    // one, or resize the best one to the requested size and cache that result.
    if (m_image)
        return m_image;
    
    return 0;
}

void IconDataCache::setImageData(unsigned char* data, int size)
{
    if (!data)
        ASSERT(!size);

    // It's okay to delete the raw image data here. Any existing clients using this icon will be
    // managing an image that was created with a copy of this raw image data.
    if (m_image)
        delete m_image;
    m_image = new Image();

    // Copy the provided data into the buffer of the new Image object
    Vector<char>& dataBuffer = m_image->dataBuffer();
    dataBuffer.resize(size);
    memcpy(dataBuffer.data(), data, size);
    
    // Tell the Image object that all of its data has been set
    if (!m_image->setData(true)) {
        LOG(IconDatabase, "Manual image data for iconURL '%s' FAILED - it was probably invalid image data", m_iconURL.ascii().data());
        delete m_image;
        m_image = 0;
    }
    
    m_dataSet = true;
}

void IconDataCache::writeToDatabase(SQLDatabase& db)
{
    if (m_iconURL.isEmpty())
        return;
    
    // First we create and prepare the SQLStatement    
    String escapedIconURL = m_iconURL;
    escapedIconURL.replace('\'', "''");
    // The following statement works no matter what in version 5 of the DB schema because we have the table
    // replace any url entry with the new data if it already exists
    SQLStatement sql(db, "INSERT INTO Icon (url,stamp,data) VALUES ('" + escapedIconURL + "', ?, ?);");
    sql.prepare();
        
    // Then we bind the timestamp
    sql.bindInt64(1, m_stamp);
        
    // Then, if we *have* data, we bind it.  Otherwise the DB will get "null" for the blob data, 
    // signifying that this icon doesn't have any data
    if (m_image && !m_image->dataBuffer().isEmpty())
        sql.bindBlob(2, m_image->dataBuffer().data(), m_image->dataBuffer().size());
    
    // Finally we step and make sure the step was successful
    if (sql.step() != SQLITE_DONE)
        LOG_ERROR("Unable to set icon data for IconURL %s", m_iconURL.ascii().data());
}

ImageDataStatus IconDataCache::imageDataStatus()
{
    if (!m_dataSet)
        return ImageDataStatusUnknown;
    if (!m_image)
        return ImageDataStatusMissing;
    return ImageDataStatusPresent;
}

    

