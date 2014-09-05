// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/base_file.h"

#include <windows.h>
#include <cguid.h>
#include <objbase.h>
#include <shellapi.h>

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/metrics/histogram.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/download/download_interrupt_reasons_impl.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/safe_util_win.h"
#include "content/public/browser/browser_thread.h"

namespace content {
namespace {

const int kAllSpecialShFileOperationCodes[] = {
  // Should be kept in sync with the case statement below.
  ERROR_ACCESS_DENIED,
  0x71,
  0x72,
  0x73,
  0x74,
  0x75,
  0x76,
  0x78,
  0x79,
  0x7A,
  0x7C,
  0x7D,
  0x7E,
  0x80,
  0x81,
  0x82,
  0x83,
  0x84,
  0x85,
  0x86,
  0x87,
  0x88,
  0xB7,
  0x402,
  0x10000,
  0x10074,
};

// Maps the result of a call to |SHFileOperation()| onto a
// |DownloadInterruptReason|.
//
// These return codes are *old* (as in, DOS era), and specific to
// |SHFileOperation()|.
// They do not appear in any windows header.
//
// See http://msdn.microsoft.com/en-us/library/bb762164(VS.85).aspx.
DownloadInterruptReason MapShFileOperationCodes(int code) {
  DownloadInterruptReason result = DOWNLOAD_INTERRUPT_REASON_NONE;

  // Check these pre-Win32 error codes first, then check for matches
  // in Winerror.h.
  // This switch statement should be kept in sync with the list of codes
  // above.
  switch (code) {
    // Not a pre-Win32 error code; here so that this particular
    // case shows up in our histograms.  This is redundant with the
    // mapping function net::MapSystemError used later.
    case ERROR_ACCESS_DENIED:  // Access is denied.
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The source and destination files are the same file.
    // DE_SAMEFILE == 0x71
    case 0x71:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The operation was canceled by the user, or silently canceled if the
    // appropriate flags were supplied to SHFileOperation.
    // DE_OPCANCELLED == 0x75
    case 0x75:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // Security settings denied access to the source.
    // DE_ACCESSDENIEDSRC == 0x78
    case 0x78:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The source or destination path exceeded or would exceed MAX_PATH.
    // DE_PATHTOODEEP == 0x79
    case 0x79:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG;
      break;

    // The path in the source or destination or both was invalid.
    // DE_INVALIDFILES == 0x7C
    case 0x7C:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The destination path is an existing file.
    // DE_FLDDESTISFILE == 0x7E
    case 0x7E:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The destination path is an existing folder.
    // DE_FILEDESTISFLD == 0x80
    case 0x80:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The name of the file exceeds MAX_PATH.
    // DE_FILENAMETOOLONG == 0x81
    case 0x81:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG;
      break;

    // The destination is a read-only CD-ROM, possibly unformatted.
    // DE_DEST_IS_CDROM == 0x82
    case 0x82:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The destination is a read-only DVD, possibly unformatted.
    // DE_DEST_IS_DVD == 0x83
    case 0x83:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The destination is a writable CD-ROM, possibly unformatted.
    // DE_DEST_IS_CDRECORD == 0x84
    case 0x84:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The file involved in the operation is too large for the destination
    // media or file system.
    // DE_FILE_TOO_LARGE == 0x85
    case 0x85:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_TOO_LARGE;
      break;

    // The source is a read-only CD-ROM, possibly unformatted.
    // DE_SRC_IS_CDROM == 0x86
    case 0x86:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The source is a read-only DVD, possibly unformatted.
    // DE_SRC_IS_DVD == 0x87
    case 0x87:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // The source is a writable CD-ROM, possibly unformatted.
    // DE_SRC_IS_CDRECORD == 0x88
    case 0x88:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
      break;

    // MAX_PATH was exceeded during the operation.
    // DE_ERROR_MAX == 0xB7
    case 0xB7:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG;
      break;

    // An unspecified error occurred on the destination.
    // XE_ERRORONDEST == 0x10000
    case 0x10000:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // Multiple file paths were specified in the source buffer, but only one
    // destination file path.
    // DE_MANYSRC1DEST == 0x72
    case 0x72:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // Rename operation was specified but the destination path is
    // a different directory. Use the move operation instead.
    // DE_DIFFDIR == 0x73
    case 0x73:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The source is a root directory, which cannot be moved or renamed.
    // DE_ROOTDIR == 0x74
    case 0x74:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The destination is a subtree of the source.
    // DE_DESTSUBTREE == 0x76
    case 0x76:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The operation involved multiple destination paths,
    // which can fail in the case of a move operation.
    // DE_MANYDEST == 0x7A
    case 0x7A:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // The source and destination have the same parent folder.
    // DE_DESTSAMETREE == 0x7D
    case 0x7D:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // An unknown error occurred.  This is typically due to an invalid path in
    // the source or destination.  This error does not occur on Windows Vista
    // and later.
    // DE_UNKNOWN_ERROR == 0x402
    case 0x402:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;

    // Destination is a root directory and cannot be renamed.
    // DE_ROOTDIR | ERRORONDEST == 0x10074
    case 0x10074:
      result = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
      break;
  }

  // Narrow down on the reason we're getting some catch-all interrupt reasons.
  if (result == DOWNLOAD_INTERRUPT_REASON_FILE_FAILED) {
    UMA_HISTOGRAM_CUSTOM_ENUMERATION(
        "Download.MapWinShErrorFileFailed", code,
        base::CustomHistogram::ArrayToCustomRanges(
            kAllSpecialShFileOperationCodes,
            arraysize(kAllSpecialShFileOperationCodes)));
  }

  if (result == DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED) {
    UMA_HISTOGRAM_CUSTOM_ENUMERATION(
        "Download.MapWinShErrorAccessDenied", code,
        base::CustomHistogram::ArrayToCustomRanges(
            kAllSpecialShFileOperationCodes,
            arraysize(kAllSpecialShFileOperationCodes)));
  }

  if (result != DOWNLOAD_INTERRUPT_REASON_NONE)
    return result;

  // If not one of the above codes, it should be a standard Windows error code.
  return ConvertNetErrorToInterruptReason(
      net::MapSystemError(code), DOWNLOAD_INTERRUPT_FROM_DISK);
}

// Maps a return code from ScanAndSaveDownloadedFile() to a
// DownloadInterruptReason. The return code in |result| is usually from the
// final IAttachmentExecute::Save() call.
DownloadInterruptReason MapScanAndSaveErrorCodeToInterruptReason(
    HRESULT result) {
  if (SUCCEEDED(result))
    return DOWNLOAD_INTERRUPT_REASON_NONE;

  switch (result) {
    case INET_E_SECURITY_PROBLEM:       // 0x800c000e
      // This is returned if the download was blocked due to security
      // restrictions. E.g. if the source URL was in the Restricted Sites zone
      // and downloads are blocked on that zone, then the download would be
      // deleted and this error code is returned.
      return DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED;

    case E_FAIL:                        // 0x80004005
      // Returned if an anti-virus product reports an infection in the
      // downloaded file during IAE::Save().
      return DOWNLOAD_INTERRUPT_REASON_FILE_VIRUS_INFECTED;

    default:
      // Any other error that occurs during IAttachmentExecute::Save() likely
      // indicates a problem with the security check, but not necessarily the
      // download. See http://crbug.com/153212.
      return DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED;
  }
}

} // namespace

// Renames a file using the SHFileOperation API to ensure that the target file
// gets the correct default security descriptor in the new path.
// Returns a network error, or net::OK for success.
DownloadInterruptReason BaseFile::MoveFileAndAdjustPermissions(
    const base::FilePath& new_path) {
  base::ThreadRestrictions::AssertIOAllowed();

  // The parameters to SHFileOperation must be terminated with 2 NULL chars.
  base::FilePath::StringType source = full_path_.value();
  base::FilePath::StringType target = new_path.value();

  source.append(1, L'\0');
  target.append(1, L'\0');

  SHFILEOPSTRUCT move_info = {0};
  move_info.wFunc = FO_MOVE;
  move_info.pFrom = source.c_str();
  move_info.pTo = target.c_str();
  move_info.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI |
      FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS;

  int result = SHFileOperation(&move_info);
  DownloadInterruptReason interrupt_reason = DOWNLOAD_INTERRUPT_REASON_NONE;

  if (result == 0 && move_info.fAnyOperationsAborted)
    interrupt_reason = DOWNLOAD_INTERRUPT_REASON_FILE_FAILED;
  else if (result != 0)
    interrupt_reason = MapShFileOperationCodes(result);

  if (interrupt_reason != DOWNLOAD_INTERRUPT_REASON_NONE)
    return LogInterruptReason("SHFileOperation", result, interrupt_reason);
  return interrupt_reason;
}

DownloadInterruptReason BaseFile::AnnotateWithSourceInformation() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(!detached_);

  bound_net_log_.BeginEvent(net::NetLog::TYPE_DOWNLOAD_FILE_ANNOTATED);
  DownloadInterruptReason result = DOWNLOAD_INTERRUPT_REASON_NONE;
  std::string braces_guid = "{" + client_guid_ + "}";
  GUID guid = GUID_NULL;
  if (base::IsValidGUID(client_guid_)) {
    HRESULT hr = CLSIDFromString(
        base::UTF8ToUTF16(braces_guid).c_str(), &guid);
    if (FAILED(hr))
      guid = GUID_NULL;
  }

  HRESULT hr = AVScanFile(full_path_, source_url_.spec(), guid);

  // If the download file is missing after the call, then treat this as an
  // interrupted download.
  //
  // If the ScanAndSaveDownloadedFile() call failed, but the downloaded file is
  // still around, then don't interrupt the download. Attachment Execution
  // Services deletes the submitted file if the downloaded file is blocked by
  // policy or if it was found to be infected.
  //
  // If the file is still there, then the error could be due to AES not being
  // available or some other error during the AES invocation. In either case,
  // we don't surface the error to the user.
  if (!base::PathExists(full_path_)) {
    DCHECK(FAILED(hr));
    result = MapScanAndSaveErrorCodeToInterruptReason(hr);
    if (result == DOWNLOAD_INTERRUPT_REASON_NONE) {
      RecordDownloadCount(FILE_MISSING_AFTER_SUCCESSFUL_SCAN_COUNT);
      result = DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED;
    }
    LogInterruptReason("ScanAndSaveDownloadedFile", hr, result);
  }
  bound_net_log_.EndEvent(net::NetLog::TYPE_DOWNLOAD_FILE_ANNOTATED);
  return result;
}

}  // namespace content
