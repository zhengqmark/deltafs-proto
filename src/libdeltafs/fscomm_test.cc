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
#include "fscomm.h"

#include "pdlfs-common/histogram.h"
#include "pdlfs-common/mutexlock.h"
#include "pdlfs-common/port.h"
#include "pdlfs-common/rpc.h"
#include "pdlfs-common/testharness.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#if defined(PDLFS_OS_LINUX)
#include <sys/resource.h>
#include <sys/time.h>
#endif

#if __cplusplus >= 201103L
#define OVERRIDE override
#else
#define OVERRIDE
#endif
namespace pdlfs {
class LokupTest : public rpc::If, public FilesystemWrapper {
 public:
  LokupTest() {
    who_.uid = 1;
    who_.gid = 2;
    parent_.SetDnodeNo(3);
    parent_.SetInodeNo(4);
    parent_.SetZerothServer(5);
    parent_.SetDirMode(6);
    parent_.SetUserId(7);
    parent_.SetGroupId(8);
    parent_.SetLeaseDue(9);
    stat_.SetDnodeNo(10);
    stat_.SetInodeNo(11);
    stat_.SetZerothServer(12);
    stat_.SetDirMode(13);
    stat_.SetUserId(14);
    stat_.SetGroupId(15);
    stat_.SetLeaseDue(16);
    name_ = "x";
  }

  virtual Status Lokup(  ///
      const User& who, const LookupStat& parent, const Slice& name,
      LookupStat* stat) OVERRIDE {
    ASSERT_EQ(who.uid, who_.uid);
    ASSERT_EQ(who.gid, who_.gid);
    ASSERT_EQ(parent.DnodeNo(), parent_.DnodeNo());
    ASSERT_EQ(parent.InodeNo(), parent_.InodeNo());
    ASSERT_EQ(parent.ZerothServer(), parent_.ZerothServer());
    ASSERT_EQ(parent.DirMode(), parent_.DirMode());
    ASSERT_EQ(parent.UserId(), parent_.UserId());
    ASSERT_EQ(parent.GroupId(), parent_.GroupId());
    ASSERT_EQ(parent.LeaseDue(), parent_.LeaseDue());
    ASSERT_EQ(name, name_);
    *stat = stat_;
    return Status::OK();
  }

  virtual Status Call(Message& in, Message& out) RPCNOEXCEPT OVERRIDE {
    return rpc::LokupOperation(this)(in, out);
  }

  LookupStat parent_;
  LookupStat stat_;
  Slice name_;
  User who_;
};

TEST(LokupTest, LokupCall) {
  LokupOptions opts;
  opts.parent = &parent_;
  opts.name = name_;
  opts.me = who_;
  LokupRet ret;
  LookupStat stat;
  ret.stat = &stat;
  ASSERT_OK(rpc::LokupCli(this)(opts, &ret));
  ASSERT_EQ(stat.DnodeNo(), stat_.DnodeNo());
  ASSERT_EQ(stat.InodeNo(), stat_.InodeNo());
  ASSERT_EQ(stat.ZerothServer(), stat_.ZerothServer());
  ASSERT_EQ(stat.DirMode(), stat_.DirMode());
  ASSERT_EQ(stat.UserId(), stat_.UserId());
  ASSERT_EQ(stat.GroupId(), stat_.GroupId());
  ASSERT_EQ(stat.LeaseDue(), stat_.LeaseDue());
}

class MkflsTest : public rpc::If, public FilesystemWrapper {
 public:
  MkflsTest() {
    who_.uid = 1;
    who_.gid = 2;
    parent_.SetDnodeNo(3);
    parent_.SetInodeNo(4);
    parent_.SetZerothServer(5);
    parent_.SetDirMode(6);
    parent_.SetUserId(7);
    parent_.SetGroupId(8);
    parent_.SetLeaseDue(9);
    mode_ = 10;
    npre_ = 11;
    n_ = 12;
    namearr_ = "x";
  }

  virtual Status Mkfls(const User& who, const LookupStat& parent,
                       const Slice& namearr, uint32_t mode,
                       uint32_t* n) OVERRIDE {
    ASSERT_EQ(who.uid, who_.uid);
    ASSERT_EQ(who.gid, who_.gid);
    ASSERT_EQ(parent.DnodeNo(), parent_.DnodeNo());
    ASSERT_EQ(parent.InodeNo(), parent_.InodeNo());
    ASSERT_EQ(parent.ZerothServer(), parent_.ZerothServer());
    ASSERT_EQ(parent.DirMode(), parent_.DirMode());
    ASSERT_EQ(parent.UserId(), parent_.UserId());
    ASSERT_EQ(parent.GroupId(), parent_.GroupId());
    ASSERT_EQ(parent.LeaseDue(), parent_.LeaseDue());
    ASSERT_EQ(namearr, namearr_);
    ASSERT_EQ(mode, mode_);
    ASSERT_EQ(*n, npre_);
    *n = n_;
    return Status::OK();
  }

  virtual Status Call(Message& in, Message& out) RPCNOEXCEPT OVERRIDE {
    return rpc::MkflsOperation(this)(in, out);
  }

  LookupStat parent_;
  Slice namearr_;
  uint32_t mode_;
  uint32_t npre_;
  uint32_t n_;
  User who_;
};

TEST(MkflsTest, MkflsCall) {
  MkflsOptions opts;
  opts.parent = &parent_;
  opts.namearr = namearr_;
  opts.n = npre_;
  opts.mode = mode_;
  opts.me = who_;
  MkflsRet ret;
  ASSERT_OK(rpc::MkflsCli(this)(opts, &ret));
  ASSERT_EQ(ret.n, n_);
}

namespace {  // RPC performance bench (the client part of it)...
// Number of concurrent threads to run.
int FLAGS_threads = 1;

// Number of rpc operations to perform per thread.
int FLAGS_num = 8;

// Print histogram of timings.
bool FLAGS_histogram = false;

// User id for the bench.
int FLAGS_uid = 1;

// Group id.
int FLAGS_gid = 1;

// Connect to the server at the following address.
const char* FLAGS_srv_uri = NULL;

// Performance stats.
class Stats {
 private:
#if defined(PDLFS_OS_LINUX)
  struct rusage start_rusage_;
  struct rusage rusage_;
#endif
  double start_;
  double finish_;
  double seconds_;
  int done_;
  int next_report_;
  int64_t bytes_;
  double last_op_finish_;
  Histogram hist_;
  std::string message_;

#if defined(PDLFS_OS_LINUX)
  static void MergeTimeval(struct timeval* tv, const struct timeval* other) {
    tv->tv_sec += other->tv_sec;
    tv->tv_usec += other->tv_usec;
  }

  static void MergeU(struct rusage* ru, const struct rusage* other) {
    MergeTimeval(&ru->ru_utime, &other->ru_utime);
    MergeTimeval(&ru->ru_stime, &other->ru_stime);
  }

  static uint64_t TimevalToMicros(const struct timeval* tv) {
    uint64_t t;
    t = static_cast<uint64_t>(tv->tv_sec) * 1000000;
    t += tv->tv_usec;
    return t;
  }
#endif

  static void AppendWithSpace(std::string* str, Slice msg) {
    if (msg.empty()) return;
    if (!str->empty()) {
      str->push_back(' ');
    }
    str->append(msg.data(), msg.size());
  }

 public:
  Stats() { Start(); }

  void Start() {
    next_report_ = 100;
    last_op_finish_ = start_;
    hist_.Clear();
    done_ = 0;
    bytes_ = 0;
    seconds_ = 0;
    start_ = CurrentMicros();
    finish_ = start_;
    message_.clear();
#if defined(PDLFS_OS_LINUX)
    getrusage(RUSAGE_THREAD, &start_rusage_);
#endif
  }

  void Merge(const Stats& other) {
#if defined(PDLFS_OS_LINUX)
    MergeU(&start_rusage_, &other.start_rusage_);
    MergeU(&rusage_, &other.rusage_);
#endif
    hist_.Merge(other.hist_);
    done_ += other.done_;
    bytes_ += other.bytes_;
    seconds_ += other.seconds_;
    if (other.start_ < start_) start_ = other.start_;
    if (other.finish_ > finish_) finish_ = other.finish_;

    // Just keep the messages from one thread
    if (message_.empty()) message_ = other.message_;
  }

  void Stop() {
#if defined(PDLFS_OS_LINUX)
    getrusage(RUSAGE_THREAD, &rusage_);
#endif
    finish_ = CurrentMicros();
    seconds_ = (finish_ - start_) * 1e-6;
  }

  void AddMessage(Slice msg) { AppendWithSpace(&message_, msg); }

  void FinishedSingleOp(int total, int tid) {
    if (FLAGS_histogram) {
      double now = CurrentMicros();
      double micros = now - last_op_finish_;
      hist_.Add(micros);
      if (micros > 20000) {
        fprintf(stderr, "Long op: %.1f micros%30s\r", micros, "");
        fflush(stderr);
      }
      last_op_finish_ = now;
    }

    done_++;
    if (tid == 0 && done_ >= next_report_) {
      if (next_report_ < 1000)
        next_report_ += 100;
      else if (next_report_ < 5000)
        next_report_ += 500;
      else if (next_report_ < 10000)
        next_report_ += 1000;
      else if (next_report_ < 50000)
        next_report_ += 5000;
      else if (next_report_ < 100000)
        next_report_ += 10000;
      else if (next_report_ < 500000)
        next_report_ += 50000;
      else
        next_report_ += 100000;
      fprintf(stderr, "... finished %d ops (%.0f%%)%30s\r", done_,
              100.0 * done_ / total, "");
      fflush(stderr);
    }
  }

  void AddBytes(int64_t n) { bytes_ += n; }

  void Report(const Slice& name) {
    // Pretend at least one op was done in case we are running a benchmark
    // that does not call FinishedSingleOp().
    if (done_ < 1) done_ = 1;

    std::string extra;
    if (bytes_ > 0) {
      // Rate is computed on actual elapsed time, not the sum of per-thread
      // elapsed times.
      double elapsed = (finish_ - start_) * 1e-6;
      char rate[100];
      snprintf(rate, sizeof(rate), "%6.1f MB/s, %.0f bytes",
               (bytes_ / 1048576.0) / elapsed, double(bytes_));
      extra = rate;
    }
    AppendWithSpace(&extra, message_);

    // Per-op latency is computed on the sum of per-thread elapsed times, not
    // the actual elapsed time.
    fprintf(stdout, "==%-12s : %16.3f micros/op, %12.0f ops;%s%s\n",
            name.ToString().c_str(), seconds_ * 1e6 / done_, double(done_),
            (extra.empty() ? "" : " "), extra.c_str());
#if defined(PDLFS_OS_LINUX)
    fprintf(stdout, "Time(usr/sys/wall): %.3f/%.3f/%.3f\n",
            (TimevalToMicros(&rusage_.ru_utime) -
             TimevalToMicros(&start_rusage_.ru_utime)) *
                1e-6,
            (TimevalToMicros(&rusage_.ru_stime) -
             TimevalToMicros(&start_rusage_.ru_stime)) *
                1e-6,
            (finish_ - start_) * 1e-6);
#endif
    if (FLAGS_histogram) {
      fprintf(stdout, "Microseconds per op:\n%s\n", hist_.ToString().c_str());
    }
    fflush(stdout);
  }
};

// State shared by all concurrent executions of the same benchmark.
struct SharedState {
  port::Mutex mu;
  port::CondVar cv;
  int total;  // Total number of threads
  // Each thread goes through the following states:
  //    (1) initializing
  //    (2) waiting for others to be initialized
  //    (3) running
  //    (4) done
  int num_initialized;
  int num_done;
  bool start;

  explicit SharedState(int total)
      : cv(&mu), total(total), num_initialized(0), num_done(0), start(false) {}
};

// Per-thread state for concurrent executions of the same benchmark.
struct ThreadState {
  int tid;  // 0..n-1 when running in n threads
  SharedState* shared;
  Stats stats;

  explicit ThreadState(int tid) : tid(tid), shared(NULL) {}
};

class Benchmark {
 private:
  RPC* rpc_;
  LookupStat parent_lstat_;
  User me_;

  static void PrintHeader() {
    PrintEnvironment();
    PrintWarnings();
    fprintf(stdout, "Threads:            %d\n", FLAGS_threads);
    fprintf(stdout, "Number requests:    %d per thread\n", FLAGS_num);
    fprintf(stdout, "Histogram:          %d\n", FLAGS_histogram);
    fprintf(stdout, "Uri:                %s\n", FLAGS_srv_uri);
    fprintf(stdout, "------------------------------------------------\n");
  }

  static void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
    fprintf(stdout, "WARNING: C++ optimization disabled\n");
#endif
#ifndef NDEBUG
    fprintf(stdout, "WARNING: C++ assertions are on\n");
#endif

    // See if snappy is working by attempting to compress a compressible string
    const char text[] = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
    std::string compressed;
    if (!port::Snappy_Compress(text, sizeof(text), &compressed)) {
      fprintf(stdout, "WARNING: Snappy compression is not enabled\n");
    } else if (compressed.size() >= sizeof(text)) {
      fprintf(stdout, "WARNING: Snappy compression is not effective\n");
    }
  }

#if defined(PDLFS_OS_LINUX)
  static Slice TrimSpace(Slice s) {
    size_t start = 0;
    while (start < s.size() && isspace(s[start])) {
      start++;
    }
    size_t limit = s.size();
    while (limit > start && isspace(s[limit - 1])) {
      limit--;
    }

    Slice r = s;
    r.remove_suffix(s.size() - limit);
    r.remove_prefix(start);
    return r;
  }
#endif

  static void PrintEnvironment() {
#if defined(PDLFS_OS_LINUX)
    time_t now = time(NULL);
    fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo != nullptr) {
      char line[1000];
      int num_cpus = 0;
      std::string cpu_type;
      std::string cache_size;
      while (fgets(line, sizeof(line), cpuinfo) != nullptr) {
        const char* sep = strchr(line, ':');
        if (sep == nullptr) {
          continue;
        }
        Slice key = TrimSpace(Slice(line, sep - 1 - line));
        Slice val = TrimSpace(Slice(sep + 1));
        if (key == "model name") {
          ++num_cpus;
          cpu_type = val.ToString();
        } else if (key == "cache size") {
          cache_size = val.ToString();
        }
      }
      fclose(cpuinfo);
      fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
      fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
    }
#endif
  }

  struct ThreadArg {
    Benchmark* bm;
    SharedState* shared;
    ThreadState* thread;
  };

  static void ThreadBody(void* v) {
    ThreadArg* const arg = reinterpret_cast<ThreadArg*>(v);
    SharedState* shared = arg->shared;
    ThreadState* thread = arg->thread;
    {
      MutexLock l(&shared->mu);
      shared->num_initialized++;
      if (shared->num_initialized >= shared->total) {
        shared->cv.SignalAll();
      }
      while (!shared->start) {
        shared->cv.Wait();
      }
    }

    thread->stats.Start();
    arg->bm->SendAndReceive(thread);
    thread->stats.Stop();

    {
      MutexLock l(&shared->mu);
      shared->num_done++;
      if (shared->num_done >= shared->total) {
        shared->cv.SignalAll();
      }
    }
  }

  void RunBenchmark(int n) {
    SharedState shared(n);

    ThreadArg* const arg = new ThreadArg[n];
    for (int i = 0; i < n; i++) {
      arg[i].bm = this;
      arg[i].shared = &shared;
      arg[i].thread = new ThreadState(i);
      arg[i].thread->shared = &shared;
      Env::Default()->StartThread(ThreadBody, &arg[i]);
    }

    {
      MutexLock ml(&shared.mu);
      while (shared.num_initialized < n) {
        shared.cv.Wait();
      }

      shared.start = true;
      shared.cv.SignalAll();
      while (shared.num_done < n) {
        shared.cv.Wait();
      }
    }

    for (int i = 1; i < n; i++) {
      arg[0].thread->stats.Merge(arg[i].thread->stats);
    }
    arg[0].thread->stats.Report("send&recieve");
    for (int i = 0; i < n; i++) {
      delete arg[i].thread;
    }
    delete[] arg;
  }

  void SendAndReceive(ThreadState* const thread) {
    MkfleOptions options;
    options.parent = &parent_lstat_;
    options.mode = 0660;
    options.me = me_;
    Stat stat;
    MkfleRet ret;
    ret.stat = &stat;
    rpc::If* rpccli = rpc_->OpenStubFor(FLAGS_srv_uri);
    rpc::MkfleCli cli(rpccli);
    Stats* const stats = &thread->stats;
    char tmp[20];
    for (int i = 0; i < FLAGS_num; i++) {
      snprintf(tmp, sizeof(tmp), "%012d", i);
      options.name = Slice(tmp, 12);
      Status s = cli(options, &ret);
      if (!s.ok()) {
        fprintf(stderr, "rpc error: %s\n", s.ToString().c_str());
        exit(1);
      }
      stats->FinishedSingleOp(FLAGS_num, thread->tid);
    }
    delete rpccli;
  }

 public:
  Benchmark() : rpc_(NULL) {
    parent_lstat_.SetDnodeNo(0);
    parent_lstat_.SetInodeNo(0);
    parent_lstat_.SetDirMode(0770 | S_IFDIR);
    parent_lstat_.SetUserId(FLAGS_uid);
    parent_lstat_.SetGroupId(FLAGS_gid);
    parent_lstat_.SetZerothServer(0);
    parent_lstat_.SetLeaseDue(-1);
    parent_lstat_.AssertAllSet();
    me_.uid = FLAGS_uid;
    me_.gid = FLAGS_gid;
  }

  ~Benchmark() {  ///
    delete rpc_;
  }

  void Run() {
    PrintHeader();
    RPCOptions opts;
    opts.uri = ":";  // Any non-empty string works
    opts.mode = rpc::kClientOnly;
    rpc_ = RPC::Open(opts);
    Status s = rpc_->status();
    if (!s.ok()) {
      fprintf(stderr, "rpc error: %s\n", s.ToString().c_str());
      exit(1);
    }

    RunBenchmark(FLAGS_threads);
  }
};
}  // namespace
}  // namespace pdlfs
#undef OVERRIDE

namespace {
void BM_Main(const int* const argc, char*** const argv) {
  std::string default_uri;

  for (int i = 2; i < *argc; i++) {
    int n;
    char junk;
    if (strncmp((*argv)[i], "--uri=", 6) == 0) {
      pdlfs::FLAGS_srv_uri = (*argv)[i] + 6;
    } else if (sscanf((*argv)[i], "--threads=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_threads = n;
    } else if (sscanf((*argv)[i], "--histogram=%d%c", &n, &junk) == 1 &&
               (n == 0 || n == 1)) {
      pdlfs::FLAGS_histogram = n;
    } else if (sscanf((*argv)[i], "--num=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_num = n;
    } else {
      fprintf(stderr, "Invalid flag: '%s'\n", (*argv)[i]);
      exit(1);
    }
  }

  if (!pdlfs::FLAGS_srv_uri) {
    default_uri = ":10086";
    pdlfs::FLAGS_srv_uri = default_uri.c_str();
  }

  pdlfs::Benchmark benchmark;
  benchmark.Run();
}
}  // namespace

int main(int argc, char* argv[]) {
  pdlfs::Slice token;
  if (argc > 1) {
    token = pdlfs::Slice(argv[1]);
  }
  if (!token.starts_with("--bench")) {
    return pdlfs::test::RunAllTests(&argc, &argv);
  } else {
    BM_Main(&argc, &argv);
    return 0;
  }
}
