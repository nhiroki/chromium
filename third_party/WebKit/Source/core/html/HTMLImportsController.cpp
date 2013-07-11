/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/html/HTMLImportsController.h"

#include "core/dom/Document.h"
#include "core/html/HTMLDocument.h"
#include "core/html/HTMLLinkElement.h"
#include "core/loader/CrossOriginAccessControl.h"
#include "core/loader/DocumentWriter.h"
#include "core/loader/cache/CachedResourceLoader.h"
#include "core/loader/cache/CachedScript.h"
#include "weborigin/SecurityOrigin.h"

namespace WebCore {

PassRefPtr<LinkImport> LinkImport::create(HTMLLinkElement* owner)
{
    return adoptRef(new LinkImport(owner));
}

LinkImport::LinkImport(HTMLLinkElement* owner)
    : LinkResource(owner)
{
}

LinkImport::~LinkImport()
{
}

Document* LinkImport::importedDocument() const
{
    if (!m_loader)
        return 0;
    return m_loader->importedDocument();
}

void LinkImport::process()
{
    if (m_loader)
        return;
    if (!m_owner)
        return;

    if (!m_owner->document()->frame() && !m_owner->document()->imports())
        return;

    LinkRequestBuilder builder(m_owner);
    if (!builder.isValid())
        return;

    HTMLImportsController* controller = m_owner->document()->imports();
    if (!controller) {
        ASSERT(m_owner->document()->frame()); // The document should be the master.
        m_owner->document()->setImports(HTMLImportsController::create(m_owner->document()));
        controller = m_owner->document()->imports();
    }

    if (RefPtr<HTMLImportLoader> found = controller->findLinkFor(builder.url())) {
        m_loader = found;
        return;
    }

    CachedResourceRequest request = builder.build(true);
    request.setPotentiallyCrossOriginEnabled(controller->securityOrigin(), DoNotAllowStoredCredentials);
    CachedResourceHandle<CachedRawResource> resource = controller->cachedResourceLoader()->requestRawResource(request);
    if (!resource)
        return;

    m_loader = HTMLImportLoader::create(controller, builder.url(), resource);
}

void LinkImport::ownerRemoved()
{
    m_owner = 0;
    m_loader.clear();
}


PassRefPtr<HTMLImportLoader> HTMLImportLoader::create(HTMLImportsController* controller, const KURL& url, const CachedResourceHandle<CachedScript>& resource)
{
    RefPtr<HTMLImportLoader> loader = adoptRef(new HTMLImportLoader(controller, url, resource));
    controller->addImport(loader);
    return loader;
}

HTMLImportLoader::HTMLImportLoader(HTMLImportsController* controller, const KURL& url, const CachedResourceHandle<CachedScript>& resource)
    : m_controller(controller)
    , m_state(StateLoading)
    , m_resource(resource)
    , m_url(url)
{
    m_resource->addClient(this);
}

HTMLImportLoader::~HTMLImportLoader()
{
    if (m_resource)
        m_resource->removeClient(this);
}

void HTMLImportLoader::responseReceived(CachedResource*, const ResourceResponse& response)
{
    setState(startParsing(response));
}

void HTMLImportLoader::dataReceived(CachedResource*, const char* data, int length)
{
    RefPtr<DocumentWriter> protectingWriter(m_writer);
    m_writer->addData(data, length);
}

void HTMLImportLoader::notifyFinished(CachedResource*)
{
    setState(finish());
}

void HTMLImportLoader::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;

    if (m_state == StateReady || m_state == StateError)
        dispose();
}

void HTMLImportLoader::dispose()
{
    if (m_writer) {
        m_writer->end();
        m_writer.clear();
    }

    if (m_resource) {
        m_resource->removeClient(this);
        m_resource = 0;
    }


    if (m_controller)
        m_controller->didLoad();
}

HTMLImportLoader::State HTMLImportLoader::startParsing(const ResourceResponse& response)
{
    // Current canAccess() implementation isn't sufficient for catching cross-domain redirects: http://crbug.com/256976
    if (!m_controller->cachedResourceLoader()->canAccess(m_resource.get()))
        return StateError;

    m_importedDocument = HTMLDocument::create(0, response.url());
    m_importedDocument->setImports(m_controller);
    m_writer = DocumentWriter::create(m_importedDocument.get(), response.mimeType(), response.textEncodingName());

    return StateLoading;
}

HTMLImportLoader::State HTMLImportLoader::finish()
{
    if (!m_controller)
        return StateError;
    // The writer instance indicates that a part of the document can be already loaded.
    // We don't take such a case as an error because the partially-loaded document has been visible from script at this point.
    if (m_resource->loadFailedOrCanceled() && !m_writer)
        return StateError;
    return StateReady;
}

Document* HTMLImportLoader::importedDocument() const
{
    if (m_state != StateReady)
        return 0;
    return m_importedDocument.get();
}

void HTMLImportLoader::importDestroyed()
{
    m_controller = 0;
    m_importedDocument.clear();
}


PassRefPtr<HTMLImportsController> HTMLImportsController::create(Document* master)
{
    return adoptRef(new HTMLImportsController(master));
}

HTMLImportsController::HTMLImportsController(Document* master)
    : m_master(master)
{
}

HTMLImportsController::~HTMLImportsController()
{
    for (size_t i = 0; i < m_imports.size(); ++i)
        m_imports[i]->importDestroyed();
}

void HTMLImportsController::addImport(PassRefPtr<HTMLImportLoader> link)
{
    ASSERT(!link->url().isEmpty() && link->url().isValid());
    m_imports.append(link);
}

void HTMLImportsController::showSecurityErrorMessage(const String& message)
{
    m_master->addConsoleMessage(JSMessageSource, ErrorMessageLevel, message);
}

void HTMLImportsController::didLoad()
{
    if (haveLoaded())
        m_master->didLoadAllImports();
}

PassRefPtr<HTMLImportLoader> HTMLImportsController::findLinkFor(const KURL& url) const
{
    for (size_t i = 0; i < m_imports.size(); ++i) {
        if (equalIgnoringFragmentIdentifier(m_imports[i]->url(), url))
            return m_imports[i];
    }

    return 0;
}

SecurityOrigin* HTMLImportsController::securityOrigin() const
{
    return m_master->securityOrigin();
}

CachedResourceLoader* HTMLImportsController::cachedResourceLoader() const
{
    return m_master->cachedResourceLoader();
}

bool HTMLImportsController::haveLoaded() const
{
    for (size_t i = 0; i < m_imports.size(); ++i) {
        if (!m_imports[i]->isDone())
            return false;
    }

    return true;
}

} // namespace WebCore
