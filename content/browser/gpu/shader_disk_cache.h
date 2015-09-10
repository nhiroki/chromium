// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_SHADER_DISK_CACHE_H_
#define CONTENT_BROWSER_GPU_SHADER_DISK_CACHE_H_

#include <map>
#include <queue>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "content/common/content_export.h"
#include "net/disk_cache/disk_cache.h"

namespace content {

class ShaderDiskCacheEntry;
class ShaderDiskReadHelper;
class ShaderClearHelper;

// ShaderDiskCache is the interface to the on disk cache for
// GL shaders.
//
// While this class is both RefCounted and SupportsWeakPtr
// when using this class you should work with the RefCounting.
// The WeakPtr is needed interally.
class CONTENT_EXPORT ShaderDiskCache
    : public base::RefCounted<ShaderDiskCache>,
      public base::SupportsWeakPtr<ShaderDiskCache> {
 public:
  void Init();

  void set_host_id(int host_id) { host_id_ = host_id; }

  // Store the |shader| into the cache under |key|.
  void Cache(const std::string& key, const std::string& shader);

  // Clear a range of entries. This supports unbounded deletes in either
  // direction by using null Time values for either |begin_time| or |end_time|.
  // The return value is a net error code. If this method returns
  // ERR_IO_PENDING, the |completion_callback| will be invoked when the
  // operation completes.
  int Clear(
      const base::Time begin_time,
      const base::Time end_time,
      const net::CompletionCallback& completion_callback);

  // Sets a callback for when the cache is available. If the cache is
  // already available the callback will not be called and net::OK is returned.
  // If the callback is set net::ERR_IO_PENDING is returned and the callback
  // will be executed when the cache is available.
  int SetAvailableCallback(const net::CompletionCallback& callback);

  // Returns the number of elements currently in the cache.
  int32 Size();

  // Set a callback notification for when all current entries have been
  // written to the cache.
  // The return value is a net error code. If this method returns
  // ERR_IO_PENDING, the |callback| will be invoked when all entries have
  // been written to the cache.
  int SetCacheCompleteCallback(const net::CompletionCallback& callback);

 private:
  friend class base::RefCounted<ShaderDiskCache>;
  friend class ShaderDiskCacheEntry;
  friend class ShaderDiskReadHelper;
  friend class ShaderCacheFactory;

  explicit ShaderDiskCache(const base::FilePath& cache_path);
  ~ShaderDiskCache();

  void CacheCreatedCallback(int rv);

  disk_cache::Backend* backend() { return backend_.get(); }

  void EntryComplete(void* entry);
  void ReadComplete();

  bool cache_available_;
  int host_id_;
  base::FilePath cache_path_;
  bool is_initialized_;
  net::CompletionCallback available_callback_;
  net::CompletionCallback cache_complete_callback_;

  scoped_ptr<disk_cache::Backend> backend_;

  scoped_refptr<ShaderDiskReadHelper> helper_;
  std::map<void*, scoped_refptr<ShaderDiskCacheEntry> > entry_map_;

  DISALLOW_COPY_AND_ASSIGN(ShaderDiskCache);
};

// ShaderCacheFactory maintains a cache of ShaderDiskCache objects
// so we only create one per profile directory.
class CONTENT_EXPORT ShaderCacheFactory {
 public:
  static ShaderCacheFactory* GetInstance();

  // Clear the shader disk cache for the given |path|. This supports unbounded
  // deletes in either direction by using null Time values for either
  // |begin_time| or |end_time|. The |callback| will be executed when the
  // clear is complete.
  void ClearByPath(const base::FilePath& path,
                   const base::Time& begin_time,
                   const base::Time& end_time,
                   const base::Closure& callback);

  // Retrieve the shader disk cache for the provided |client_id|.
  scoped_refptr<ShaderDiskCache> Get(int32 client_id);

  // Set the |path| to be used for the disk cache for |client_id|.
  void SetCacheInfo(int32 client_id, const base::FilePath& path);

  // Remove the path mapping for |client_id|.
  void RemoveCacheInfo(int32 client_id);

  // Set the provided |cache| into the cache map for the given |path|.
  void AddToCache(const base::FilePath& path, ShaderDiskCache* cache);

  // Remove the provided |path| from our cache map.
  void RemoveFromCache(const base::FilePath& path);

 private:
  friend struct base::DefaultSingletonTraits<ShaderCacheFactory>;
  friend class ShaderClearHelper;

  ShaderCacheFactory();
  ~ShaderCacheFactory();

  scoped_refptr<ShaderDiskCache> GetByPath(const base::FilePath& path);
  void CacheCleared(const base::FilePath& path);

  typedef std::map<base::FilePath, ShaderDiskCache*> ShaderCacheMap;
  ShaderCacheMap shader_cache_map_;

  typedef std::map<int32, base::FilePath> ClientIdToPathMap;
  ClientIdToPathMap client_id_to_path_map_;

  typedef std::queue<scoped_refptr<ShaderClearHelper> > ShaderClearQueue;
  typedef std::map<base::FilePath, ShaderClearQueue> ShaderClearMap;
  ShaderClearMap shader_clear_map_;

  DISALLOW_COPY_AND_ASSIGN(ShaderCacheFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_SHADER_DISK_CACHE_H_

