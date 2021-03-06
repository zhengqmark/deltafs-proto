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

#include "env_wrapper.h"

#include "pdlfs-common/leveldb/db.h"
#include "pdlfs-common/leveldb/filter_policy.h"
#include "pdlfs-common/leveldb/options.h"
#include "pdlfs-common/leveldb/readonly.h"
#include "pdlfs-common/leveldb/snapshot.h"
#include "pdlfs-common/leveldb/write_batch.h"

#include "pdlfs-common/cache.h"
#include "pdlfs-common/fsdb0.h"
#include "pdlfs-common/strutil.h"

#include <stdlib.h>

namespace pdlfs {
namespace {
typedef MXDB<DB, Slice, Status, kNameInKey> MDB;
}

FilesystemDbStats::FilesystemDbStats()
    : putkeybytes(0),
      putbytes(0),
      puts(0),
      getkeybytes(0),
      getbytes(0),
      gets(0) {}

void FilesystemDbStats::Merge(const FilesystemDbStats& other) {
  putkeybytes += other.putkeybytes;
  putbytes += other.putbytes;
  puts += other.puts;
  getkeybytes += other.getkeybytes;
  getbytes += other.getbytes;
  gets += other.gets;
}

FilesystemDbOptions::FilesystemDbOptions()
    : write_ahead_log_buffer(4 << 10),
      manifest_buffer(4 << 10),
      table_buffer(256 << 10),
      table_bulk_read_size(256 << 10),
      memtable_size(8 << 20),
      table_size(4 << 20),
      block_size(4 << 10),
      table_cache_size(2500),
      filter_bits_per_key(10),
      block_cache_size(0),
      block_restart_interval(16),
      level_factor(8),
      l1_compaction_trigger(5),
      l0_compaction_trigger(4),
      l0_soft_limit(8),
      l0_hard_limit(12),
      detach_dir_on_close(false),
      detach_dir_on_bulk_end(false),
      attach_dir_on_bulk(false),
      create_dir_on_bulk(false),
      bulk_use_copy(false),
      enable_io_monitoring(false),
      use_default_logger(false),
      disable_write_ahead_logging(false),
      disable_compaction(false),
      compression(false) {}

namespace {
template <typename T>
void ReadIntegerOptionFromEnv(const char* key, T* const dst) {
  const char* env = getenv(key);
  if (!env || !env[0]) return;
  uint64_t tmp;
  if (ParsePrettyNumber(env, &tmp)) {
    *dst = static_cast<T>(tmp);
  }
}

void ReadBoolFromEnv(const char* key, bool* dst) {
  const char* env = getenv(key);
  if (!env || !env[0]) return;
  bool tmp;
  if (ParsePrettyBool(env, &tmp)) {
    *dst = tmp;
  }
}
}  // namespace

// Read options from system env. All env keys start with "DELTAFS_Db_".
void FilesystemDbOptions::ReadFromEnv() {
  ReadIntegerOptionFromEnv("DELTAFS_Db_write_ahead_log_buffer",
                           &write_ahead_log_buffer);
  ReadIntegerOptionFromEnv("DELTAFS_Db_manifest_buffer", &manifest_buffer);
  ReadIntegerOptionFromEnv("DELTAFS_Db_table_buffer", &table_buffer);
  ReadIntegerOptionFromEnv("DELTAFS_Db_memtable_size", &memtable_size);
  ReadIntegerOptionFromEnv("DELTAFS_Db_table_size", &table_size);
  ReadIntegerOptionFromEnv("DELTAFS_Db_block_size", &block_size);
  ReadIntegerOptionFromEnv("DELTAFS_Db_filter_bits_per_key",
                           &filter_bits_per_key);
  ReadIntegerOptionFromEnv("DELTAFS_Db_table_cache_size", &table_cache_size);
  ReadIntegerOptionFromEnv("DELTAFS_Db_block_cache_size", &block_cache_size);
  ReadIntegerOptionFromEnv("DELTAFS_Db_block_restart_interval",
                           &block_restart_interval);
  ReadIntegerOptionFromEnv("DELTAFS_Db_level_factor", &level_factor);
  ReadIntegerOptionFromEnv("DELTAFS_Db_l1_compaction_trigger",
                           &l1_compaction_trigger);
  ReadIntegerOptionFromEnv("DELTAFS_Db_l0_compaction_trigger",
                           &l0_compaction_trigger);
  ReadIntegerOptionFromEnv("DELTAFS_Db_l0_soft_limit", &l0_soft_limit);
  ReadIntegerOptionFromEnv("DELTAFS_Db_l0_hard_limit", &l0_hard_limit);
  ReadBoolFromEnv("DELTAFS_Db_use_default_logger", &use_default_logger);
  ReadBoolFromEnv("DELTAFS_Db_disable_write_ahead_logging",
                  &disable_write_ahead_logging);
  ReadBoolFromEnv("DELTAFS_Db_disable_compaction", &disable_compaction);
  ReadBoolFromEnv("DELTAFS_Db_enable_io_monitoring", &enable_io_monitoring);
  ReadBoolFromEnv("DELTAFS_Db_compression", &compression);
}

Status FilesystemDb::ReadonlyOpen(const std::string& dbloc) {
  DBOptions dbopts;
  dbopts.create_if_missing = false;
  dbopts.detach_dir_on_close = options_.detach_dir_on_close;
  dbopts.table_cache = table_cache_;
  dbopts.block_cache = block_cache_;
  dbopts.filter_policy = filter_policy_;
  dbopts.info_log = options_.use_default_logger ? Logger::Default() : NULL;
  myenv_->SetDbLoc(dbloc);
  dbopts.env = myenv_;
  Status status = ReadonlyDB::Open(dbopts, dbloc, &db_);
  if (status.ok()) {
    mdb_ = reinterpret_cast<MetadataDb*>(new MDB(db_));
  }
  return status;
}

Status FilesystemDb::Open(const std::string& dbloc, bool readonly) {
  if (readonly) {
    return ReadonlyOpen(dbloc);
  }
  DBOptions dbopts;
  dbopts.create_if_missing = true;
  dbopts.table_builder_skip_verification = true;
  dbopts.sync_log_on_close = true;
  dbopts.detach_dir_on_close = options_.detach_dir_on_close;
  dbopts.disable_write_ahead_log = options_.disable_write_ahead_logging;
  dbopts.prefetch_compaction_input = options_.prefetch_compaction_input;
  dbopts.disable_compaction = options_.disable_compaction;
  dbopts.disable_seek_compaction = true;
  dbopts.rotating_manifest = true;
  dbopts.skip_lock_file = true;
  dbopts.table_cache = table_cache_;
  dbopts.block_cache = block_cache_;
  dbopts.filter_policy = filter_policy_;
  dbopts.table_bulk_read_size = options_.table_bulk_read_size;
  dbopts.write_buffer_size = options_.memtable_size;
  dbopts.table_file_size = options_.table_size;
  dbopts.block_size = options_.block_size;
  dbopts.block_restart_interval = options_.block_restart_interval;
  dbopts.level_factor = options_.level_factor;
  dbopts.l1_compaction_trigger = options_.l1_compaction_trigger;
  dbopts.l0_compaction_trigger = options_.l0_compaction_trigger;
  dbopts.l0_soft_limit = options_.l0_soft_limit;
  dbopts.l0_hard_limit = options_.l0_hard_limit;
  dbopts.max_mem_compact_level = 0;
  dbopts.info_log = options_.use_default_logger ? Logger::Default() : NULL;
  dbopts.compression =
      options_.compression ? kSnappyCompression : kNoCompression;
  myenv_->SetDbLoc(dbloc);
  dbopts.env = myenv_;
  Status status = DB::Open(dbopts, dbloc, &db_);
  if (status.ok()) {
    mdb_ = reinterpret_cast<MetadataDb*>(new MDB(db_));
  }
  return status;
}

Status FilesystemDb::DestroyDb(  ///
    const std::string& dbloc, Env* const env) {
  if (env) {
    // XXX: The following code forces the db dir to be mounted in
    // case that the underlying env is an object store. Created
    // dir will eventually be deleted by the subsequent
    // DestroyDB() so no harm will be done.
    //
    // When env is NULL, this step is unnecessary because
    // Env::Default() will be used which does not require a pre-mount.
    env->CreateDir(dbloc.c_str());
  }
  DBOptions dbopts;
  dbopts.skip_lock_file = true;
  dbopts.env = env;
  return DestroyDB(dbloc, dbopts);
}

struct FilesystemDb::Tx {
  const Snapshot* snap;
  WriteBatch bat;
};

FilesystemDb::FilesystemDb(const FilesystemDbOptions& options, Env* base)
    : mdb_(NULL),
      options_(options),
      myenv_(new FilesystemDbEnvWrapper(options, base)),
      filter_policy_(options_.filter_bits_per_key != 0
                         ? NewBloomFilterPolicy(options_.filter_bits_per_key)
                         : NULL),
      table_cache_(NewLRUCache(options_.table_cache_size)),
      block_cache_(NewLRUCache(options_.block_cache_size)),
      db_(NULL) {}

FilesystemDb::~FilesystemDb() {
  delete reinterpret_cast<MDB*>(mdb_);
  delete db_;
  delete filter_policy_;
  delete block_cache_;
  delete table_cache_;
  delete myenv_;
}

namespace {
struct FlushState {
  FlushOptions opts;
  DB* db;
};

void FlushDb(void* arg) {
  FlushState* state = reinterpret_cast<FlushState*>(arg);
  state->db->FlushMemTable(state->opts);
  free(state);
}

}  // namespace

Status FilesystemDb::Flush(bool force_l0, bool async) {
  Status s;
  FlushOptions fopts;
  fopts.force_flush_l0 = force_l0;
  if (async) {
    FlushState* const state =
        static_cast<FlushState*>(malloc(sizeof(FlushState)));
    state->opts = fopts;
    state->db = db_;
    Env::Default()->StartThread(FlushDb, state);
  } else {
    s = db_->FlushMemTable(fopts);
  }
  return s;
}

Status FilesystemDb::Put(  ///
    const DirId& id, const Slice& fname, const Stat& stat,
    FilesystemDbStats* const stats) {
  WriteOptions options;
  Tx* const tx = NULL;
  return reinterpret_cast<MDB*>(mdb_)->PUT<Key>(  ///
      id, fname, stat, fname, &options, tx, stats);
}

Status FilesystemDb::Get(  ///
    const DirId& id, const Slice& fname, Stat* const stat,
    FilesystemDbStats* const stats) {
  ReadOptions options;
  Tx* const tx = NULL;
  return reinterpret_cast<MDB*>(mdb_)->GET<Key>(id, fname, stat, NULL, &options,
                                                tx, stats);
}

Status FilesystemDb::Delete(const DirId& id, const Slice& fname) {
  WriteOptions options;
  Tx* const tx = NULL;
  return reinterpret_cast<MDB*>(mdb_)->DELETE<Key>(id, fname, &options, tx);
}

Status FilesystemDb::BulkInsert(const std::string& dir) {
  if (options_.create_dir_on_bulk) {
    myenv_->CreateDir(dir.c_str());
  }
  InsertOptions options(options_.bulk_use_copy ? kCopy : kRename);
  options.attach_dir_on_start = options_.attach_dir_on_bulk;
  options.detach_dir_on_complete = options_.detach_dir_on_bulk_end;
  return db_->AddL0Tables(options, dir);
}

std::string FilesystemDb::GetDbLevel0Events() {
  std::string tmp;
  db_->GetProperty("leveldb.l0-events", &tmp);
  return tmp;
}

std::string FilesystemDb::GetDbStats() {
  std::string tmp;
  db_->GetProperty("leveldb.stats", &tmp);
  return tmp;
}

}  // namespace pdlfs
