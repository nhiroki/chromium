// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_MEDIA_GALLERIES_PMP_COLUMN_READER_H_
#define CHROME_UTILITY_MEDIA_GALLERIES_PMP_COLUMN_READER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/common/media_galleries/pmp_constants.h"

namespace base {
class File;
class FilePath;
}

namespace picasa {

// Reads a single PMP column from a file.
class PmpColumnReader {
 public:
  PmpColumnReader();
  virtual ~PmpColumnReader();

  // Returns true if read successfully.
  // |rows_read| is undefined if returns false.
  bool ReadFile(base::File* file, const PmpFieldType expected_type);

  // These functions read the value of that |row| into |result|.
  // Functions return false if the column is of the wrong type or the row
  // is out of range. May only be called after successful ReadColumn.
  bool ReadString(const uint32 row, std::string* result) const;
  bool ReadUInt32(const uint32 row, uint32* result) const;
  bool ReadDouble64(const uint32 row, double* result) const;
  bool ReadUInt8(const uint32 row, uint8* result) const;
  bool ReadUInt64(const uint32 row, uint64* result) const;

  // May only be called after successful ReadColumn.
  uint32 rows_read() const;

 private:
  bool ParseData(const PmpFieldType expected_type);
  // Returns the number of bytes parsed in the body, or, -1 on failure.
  int64 IndexStrings();

  // Source data
  scoped_ptr<uint8[]> data_;
  int64 length_;

  // Header data
  PmpFieldType field_type_;
  uint32 rows_read_;

  // Index of string start locations if fields are strings. Empty otherwise.
  std::vector<const char*> strings_;

  DISALLOW_COPY_AND_ASSIGN(PmpColumnReader);
};

}  // namespace picasa

#endif  // CHROME_UTILITY_MEDIA_GALLERIES_PMP_COLUMN_READER_H_
