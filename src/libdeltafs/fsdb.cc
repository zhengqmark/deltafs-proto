/*
 * Copyright (c) 2019 Carnegie Mellon University,
 * Copyright (c) 2019 Triad National Security, LLC, as operator of
 *     Los Alamos National Laboratory.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of CMU, TRIAD, Los Alamos National Laboratory, LANL, the
 *    U.S. Government, nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsdb.h"

#include "pdlfs-common/cache.h"

namespace pdlfs {

FilesystemDbOptions::FilesystemDbOptions()
    : filter_bits_per_key(12), block_cache_size(8u << 20u) {}

Status FilesystemDb::Open(const std::string& dbloc) {
  DBOptions options;
  options.error_if_exists = options.create_if_missing = true;
  options.block_cache = block_cache_;
  options.filter_policy = filter_;
  Status status = DB::Open(options, dbloc, &db_);
  if (status.ok()) {
    mdb_ = new MDB(db_);
  }
  return status;
}

struct FilesystemDb::Tx {
  const Snapshot* snap;
  WriteBatch bat;
};

FilesystemDb::FilesystemDb(const FilesystemDbOptions& options)
    : mdb_(NULL),
      options_(options),
      filter_(NewBloomFilterPolicy(options_.filter_bits_per_key)),
      block_cache_(NewLRUCache(options_.block_cache_size)),
      db_(NULL) {}

FilesystemDb::~FilesystemDb() {
  delete db_;
  delete block_cache_;
  delete filter_;
  delete mdb_;
}

Status FilesystemDb::Flush() {  ///
  return db_->FlushMemTable(FlushOptions());
}

Status FilesystemDb::Set(  ///
    const DirId& id, const Slice& fname, const Stat& stat) {
  WriteOptions options;
  Tx* const tx = NULL;
  return mdb_->SET<Key>(id, fname, stat, fname, &options, tx);
}

Status FilesystemDb::Get(const DirId& id, const Slice& fname, Stat* stat) {
  ReadOptions options;
  Tx* const tx = NULL;
  return mdb_->GET<Key>(id, fname, stat, NULL, &options, tx);
}

Status FilesystemDb::Delete(const DirId& id, const Slice& fname) {
  WriteOptions options;
  Tx* const tx = NULL;
  return mdb_->DELETE<Key>(id, fname, &options, tx);
}

}  // namespace pdlfs
