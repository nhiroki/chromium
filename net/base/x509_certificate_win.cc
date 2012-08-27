// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/x509_certificate.h"

#include <blapi.h>  // Implement CalculateChainFingerprint() with NSS.

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/pickle.h"
#include "base/sha1.h"
#include "base/string_tokenizer.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "crypto/capi_util.h"
#include "crypto/rsa_private_key.h"
#include "crypto/scoped_capi_types.h"
#include "net/base/net_errors.h"

#pragma comment(lib, "crypt32.lib")

using base::Time;

namespace net {

namespace {

typedef crypto::ScopedCAPIHandle<
    HCERTSTORE,
    crypto::CAPIDestroyerWithFlags<HCERTSTORE,
                                   CertCloseStore, 0> > ScopedHCERTSTORE;

void ExplodedTimeToSystemTime(const base::Time::Exploded& exploded,
                              SYSTEMTIME* system_time) {
  system_time->wYear = exploded.year;
  system_time->wMonth = exploded.month;
  system_time->wDayOfWeek = exploded.day_of_week;
  system_time->wDay = exploded.day_of_month;
  system_time->wHour = exploded.hour;
  system_time->wMinute = exploded.minute;
  system_time->wSecond = exploded.second;
  system_time->wMilliseconds = exploded.millisecond;
}

//-----------------------------------------------------------------------------

// Decodes the cert's subjectAltName extension into a CERT_ALT_NAME_INFO
// structure and stores it in *output.
void GetCertSubjectAltName(PCCERT_CONTEXT cert,
                           scoped_ptr_malloc<CERT_ALT_NAME_INFO>* output) {
  PCERT_EXTENSION extension = CertFindExtension(szOID_SUBJECT_ALT_NAME2,
                                                cert->pCertInfo->cExtension,
                                                cert->pCertInfo->rgExtension);
  if (!extension)
    return;

  CRYPT_DECODE_PARA decode_para;
  decode_para.cbSize = sizeof(decode_para);
  decode_para.pfnAlloc = crypto::CryptAlloc;
  decode_para.pfnFree = crypto::CryptFree;
  CERT_ALT_NAME_INFO* alt_name_info = NULL;
  DWORD alt_name_info_size = 0;
  BOOL rv;
  rv = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           szOID_SUBJECT_ALT_NAME2,
                           extension->Value.pbData,
                           extension->Value.cbData,
                           CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
                           &decode_para,
                           &alt_name_info,
                           &alt_name_info_size);
  if (rv)
    output->reset(alt_name_info);
}

void AddCertsFromStore(HCERTSTORE store,
                       X509Certificate::OSCertHandles* results) {
  PCCERT_CONTEXT cert = NULL;

  while ((cert = CertEnumCertificatesInStore(store, cert)) != NULL) {
    PCCERT_CONTEXT to_add = NULL;
    if (CertAddCertificateContextToStore(
        NULL,  // The cert won't be persisted in any cert store. This breaks
               // any association the context currently has to |store|, which
               // allows us, the caller, to safely close |store| without
               // releasing the cert handles.
        cert,
        CERT_STORE_ADD_USE_EXISTING,
        &to_add) && to_add != NULL) {
      // When processing stores generated from PKCS#7/PKCS#12 files, it
      // appears that the order returned is the inverse of the order that it
      // appeared in the file.
      // TODO(rsleevi): Ensure this order is consistent across all Win
      // versions
      results->insert(results->begin(), to_add);
    }
  }
}

X509Certificate::OSCertHandles ParsePKCS7(const char* data, size_t length) {
  X509Certificate::OSCertHandles results;
  CERT_BLOB data_blob;
  data_blob.cbData = length;
  data_blob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data));

  HCERTSTORE out_store = NULL;

  DWORD expected_types = CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                         CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED |
                         CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED;

  if (!CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &data_blob, expected_types,
                        CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL,
                        &out_store, NULL, NULL) || out_store == NULL) {
    return results;
  }

  AddCertsFromStore(out_store, &results);
  CertCloseStore(out_store, CERT_CLOSE_STORE_CHECK_FLAG);

  return results;
}

}  // namespace

void X509Certificate::Initialize() {
  DCHECK(cert_handle_);
  subject_.ParseDistinguishedName(cert_handle_->pCertInfo->Subject.pbData,
                                  cert_handle_->pCertInfo->Subject.cbData);
  issuer_.ParseDistinguishedName(cert_handle_->pCertInfo->Issuer.pbData,
                                 cert_handle_->pCertInfo->Issuer.cbData);

  valid_start_ = Time::FromFileTime(cert_handle_->pCertInfo->NotBefore);
  valid_expiry_ = Time::FromFileTime(cert_handle_->pCertInfo->NotAfter);

  fingerprint_ = CalculateFingerprint(cert_handle_);
  ca_fingerprint_ = CalculateCAFingerprint(intermediate_ca_certs_);

  const CRYPT_INTEGER_BLOB* serial = &cert_handle_->pCertInfo->SerialNumber;
  scoped_array<uint8> serial_bytes(new uint8[serial->cbData]);
  for (unsigned i = 0; i < serial->cbData; i++)
    serial_bytes[i] = serial->pbData[serial->cbData - i - 1];
  serial_number_ = std::string(
      reinterpret_cast<char*>(serial_bytes.get()), serial->cbData);
}

// static
X509Certificate* X509Certificate::CreateSelfSigned(
    crypto::RSAPrivateKey* key,
    const std::string& subject,
    uint32 serial_number,
    base::TimeDelta valid_duration) {
  // Get the ASN.1 encoding of the certificate subject.
  std::wstring w_subject = ASCIIToWide(subject);
  DWORD encoded_subject_length = 0;
  if (!CertStrToName(
          X509_ASN_ENCODING,
          w_subject.c_str(),
          CERT_X500_NAME_STR, NULL, NULL, &encoded_subject_length, NULL)) {
    return NULL;
  }

  scoped_array<BYTE> encoded_subject(new BYTE[encoded_subject_length]);
  if (!CertStrToName(
          X509_ASN_ENCODING,
          w_subject.c_str(),
          CERT_X500_NAME_STR, NULL,
          encoded_subject.get(),
          &encoded_subject_length, NULL)) {
    return NULL;
  }

  CERT_NAME_BLOB subject_name;
  memset(&subject_name, 0, sizeof(subject_name));
  subject_name.cbData = encoded_subject_length;
  subject_name.pbData = encoded_subject.get();

  CRYPT_ALGORITHM_IDENTIFIER sign_algo;
  memset(&sign_algo, 0, sizeof(sign_algo));
  sign_algo.pszObjId = szOID_RSA_SHA1RSA;

  base::Time not_before = base::Time::Now();
  base::Time not_after = not_before + valid_duration;
  base::Time::Exploded exploded;

  // Create the system time structs representing our exploded times.
  not_before.UTCExplode(&exploded);
  SYSTEMTIME start_time;
  ExplodedTimeToSystemTime(exploded, &start_time);
  not_after.UTCExplode(&exploded);
  SYSTEMTIME end_time;
  ExplodedTimeToSystemTime(exploded, &end_time);

  PCCERT_CONTEXT cert_handle =
      CertCreateSelfSignCertificate(key->provider(), &subject_name,
                                    CERT_CREATE_SELFSIGN_NO_KEY_INFO, NULL,
                                    &sign_algo, &start_time, &end_time, NULL);
  DCHECK(cert_handle) << "Failed to create self-signed certificate: "
                      << GetLastError();
  if (!cert_handle)
    return NULL;

  X509Certificate* cert = CreateFromHandle(cert_handle, OSCertHandles());
  FreeOSCertHandle(cert_handle);
  return cert;
}

void X509Certificate::GetSubjectAltName(
    std::vector<std::string>* dns_names,
    std::vector<std::string>* ip_addrs) const {
  if (dns_names)
    dns_names->clear();
  if (ip_addrs)
    ip_addrs->clear();

  if (!cert_handle_)
    return;

  scoped_ptr_malloc<CERT_ALT_NAME_INFO> alt_name_info;
  GetCertSubjectAltName(cert_handle_, &alt_name_info);
  CERT_ALT_NAME_INFO* alt_name = alt_name_info.get();
  if (alt_name) {
    int num_entries = alt_name->cAltEntry;
    for (int i = 0; i < num_entries; i++) {
      // dNSName is an ASN.1 IA5String representing a string of ASCII
      // characters, so we can use WideToASCII here.
      const CERT_ALT_NAME_ENTRY& entry = alt_name->rgAltEntry[i];

      if (dns_names && entry.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME) {
        dns_names->push_back(WideToASCII(entry.pwszDNSName));
      } else if (ip_addrs &&
                 entry.dwAltNameChoice == CERT_ALT_NAME_IP_ADDRESS) {
        ip_addrs->push_back(std::string(
            reinterpret_cast<const char*>(entry.IPAddress.pbData),
            entry.IPAddress.cbData));
      }
    }
  }
}

PCCERT_CONTEXT X509Certificate::CreateOSCertChainForCert() const {
  // Create an in-memory certificate store to hold this certificate and
  // any intermediate certificates in |intermediate_ca_certs_|. The store
  // will be referenced in the returned PCCERT_CONTEXT, and will not be freed
  // until the PCCERT_CONTEXT is freed.
  ScopedHCERTSTORE store(CertOpenStore(
      CERT_STORE_PROV_MEMORY, 0, NULL,
      CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, NULL));
  if (!store.get())
    return NULL;

  // NOTE: This preserves all of the properties of |os_cert_handle()| except
  // for CERT_KEY_PROV_HANDLE_PROP_ID and CERT_KEY_CONTEXT_PROP_ID - the two
  // properties that hold access to already-opened private keys. If a handle
  // has already been unlocked (eg: PIN prompt), then the first time that the
  // identity is used for client auth, it may prompt the user again.
  PCCERT_CONTEXT primary_cert;
  BOOL ok = CertAddCertificateContextToStore(store.get(), os_cert_handle(),
                                             CERT_STORE_ADD_ALWAYS,
                                             &primary_cert);
  if (!ok || !primary_cert)
    return NULL;

  for (size_t i = 0; i < intermediate_ca_certs_.size(); ++i) {
    CertAddCertificateContextToStore(store.get(), intermediate_ca_certs_[i],
                                     CERT_STORE_ADD_ALWAYS, NULL);
  }

  // Note: |store| is explicitly not released, as the call to CertCloseStore()
  // when |store| goes out of scope will not actually free the store. Instead,
  // the store will be freed when |primary_cert| is freed.
  return primary_cert;
}

// static
bool X509Certificate::GetDEREncoded(X509Certificate::OSCertHandle cert_handle,
                                    std::string* encoded) {
  if (!cert_handle->pbCertEncoded || !cert_handle->cbCertEncoded)
    return false;
  encoded->assign(reinterpret_cast<char*>(cert_handle->pbCertEncoded),
                  cert_handle->cbCertEncoded);
  return true;
}

// static
bool X509Certificate::IsSameOSCert(X509Certificate::OSCertHandle a,
                                   X509Certificate::OSCertHandle b) {
  DCHECK(a && b);
  if (a == b)
    return true;
  return a->cbCertEncoded == b->cbCertEncoded &&
      memcmp(a->pbCertEncoded, b->pbCertEncoded, a->cbCertEncoded) == 0;
}

// static
X509Certificate::OSCertHandle X509Certificate::CreateOSCertHandleFromBytes(
    const char* data, int length) {
  OSCertHandle cert_handle = NULL;
  if (!CertAddEncodedCertificateToStore(
      NULL, X509_ASN_ENCODING, reinterpret_cast<const BYTE*>(data),
      length, CERT_STORE_ADD_USE_EXISTING, &cert_handle))
    return NULL;

  return cert_handle;
}

X509Certificate::OSCertHandles X509Certificate::CreateOSCertHandlesFromBytes(
    const char* data, int length, Format format) {
  OSCertHandles results;
  switch (format) {
    case FORMAT_SINGLE_CERTIFICATE: {
      OSCertHandle handle = CreateOSCertHandleFromBytes(data, length);
      if (handle != NULL)
        results.push_back(handle);
      break;
    }
    case FORMAT_PKCS7:
      results = ParsePKCS7(data, length);
      break;
    default:
      NOTREACHED() << "Certificate format " << format << " unimplemented";
      break;
  }

  return results;
}

// static
X509Certificate::OSCertHandle X509Certificate::DupOSCertHandle(
    OSCertHandle cert_handle) {
  return CertDuplicateCertificateContext(cert_handle);
}

// static
void X509Certificate::FreeOSCertHandle(OSCertHandle cert_handle) {
  CertFreeCertificateContext(cert_handle);
}

// static
SHA1Fingerprint X509Certificate::CalculateFingerprint(
    OSCertHandle cert) {
  DCHECK(NULL != cert->pbCertEncoded);
  DCHECK_NE(static_cast<DWORD>(0), cert->cbCertEncoded);

  BOOL rv;
  SHA1Fingerprint sha1;
  DWORD sha1_size = sizeof(sha1.data);
  rv = CryptHashCertificate(NULL, CALG_SHA1, 0, cert->pbCertEncoded,
                            cert->cbCertEncoded, sha1.data, &sha1_size);
  DCHECK(rv && sha1_size == sizeof(sha1.data));
  if (!rv)
    memset(sha1.data, 0, sizeof(sha1.data));
  return sha1;
}

// TODO(wtc): This function is implemented with NSS low-level hash
// functions to ensure it is fast.  Reimplement this function with
// CryptoAPI.  May need to cache the HCRYPTPROV to reduce the overhead.
// static
SHA1Fingerprint X509Certificate::CalculateCAFingerprint(
    const OSCertHandles& intermediates) {
  SHA1Fingerprint sha1;
  memset(sha1.data, 0, sizeof(sha1.data));

  SHA1Context* sha1_ctx = SHA1_NewContext();
  if (!sha1_ctx)
    return sha1;
  SHA1_Begin(sha1_ctx);
  for (size_t i = 0; i < intermediates.size(); ++i) {
    PCCERT_CONTEXT ca_cert = intermediates[i];
    SHA1_Update(sha1_ctx, ca_cert->pbCertEncoded, ca_cert->cbCertEncoded);
  }
  unsigned int result_len;
  SHA1_End(sha1_ctx, sha1.data, &result_len, SHA1_LENGTH);
  SHA1_DestroyContext(sha1_ctx, PR_TRUE);

  return sha1;
}

// static
X509Certificate::OSCertHandle
X509Certificate::ReadOSCertHandleFromPickle(PickleIterator* pickle_iter) {
  const char* data;
  int length;
  if (!pickle_iter->ReadData(&data, &length))
    return NULL;

  // Legacy serialized certificates were serialized with extended attributes,
  // rather than as DER only. As a result, these serialized certificates are
  // not portable across platforms and may have side-effects on Windows due
  // to extended attributes being serialized/deserialized -
  // http://crbug.com/118706. To avoid deserializing these attributes, write
  // the deserialized cert into a temporary cert store and then create a new
  // cert from the DER - that is, without attributes.
  ScopedHCERTSTORE store(
      CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL));
  if (!store.get())
    return NULL;

  OSCertHandle cert_handle = NULL;
  if (!CertAddSerializedElementToStore(
          store.get(), reinterpret_cast<const BYTE*>(data), length,
          CERT_STORE_ADD_NEW, 0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
          NULL, reinterpret_cast<const void **>(&cert_handle))) {
    return NULL;
  }

  std::string encoded;
  bool ok = GetDEREncoded(cert_handle, &encoded);
  FreeOSCertHandle(cert_handle);
  cert_handle = NULL;

  if (ok)
    cert_handle = CreateOSCertHandleFromBytes(encoded.data(), encoded.size());
  return cert_handle;
}

// static
bool X509Certificate::WriteOSCertHandleToPickle(OSCertHandle cert_handle,
                                                Pickle* pickle) {
  return pickle->WriteData(
      reinterpret_cast<char*>(cert_handle->pbCertEncoded),
      cert_handle->cbCertEncoded);
}

// static
void X509Certificate::GetPublicKeyInfo(OSCertHandle cert_handle,
                                       size_t* size_bits,
                                       PublicKeyType* type) {
  *type = kPublicKeyTypeUnknown;
  *size_bits = 0;

  PCCRYPT_OID_INFO oid_info = CryptFindOIDInfo(
      CRYPT_OID_INFO_OID_KEY,
      cert_handle->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId,
      CRYPT_PUBKEY_ALG_OID_GROUP_ID);
  if (!oid_info)
    return;

  CHECK_EQ(oid_info->dwGroupId,
           static_cast<DWORD>(CRYPT_PUBKEY_ALG_OID_GROUP_ID));

  *size_bits = CertGetPublicKeyLength(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      &cert_handle->pCertInfo->SubjectPublicKeyInfo);

  switch (oid_info->Algid) {
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
      *type = kPublicKeyTypeRSA;
      break;
    case CALG_DSS_SIGN:
      *type = kPublicKeyTypeDSA;
      break;
    case CALG_ECDSA:
      *type = kPublicKeyTypeECDSA;
      break;
    case CALG_ECDH:
      *type = kPublicKeyTypeECDH;
      break;
  }
}

}  // namespace net
