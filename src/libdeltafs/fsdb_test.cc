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

#include "pdlfs-common/env.h"
#include "pdlfs-common/histogram.h"
#include "pdlfs-common/mutexlock.h"
#include "pdlfs-common/testharness.h"

#include <stdio.h>

namespace pdlfs {

class FilesystemDbTest {
 public:
  FilesystemDbTest() : dbloc_(test::TmpDir() + "/fsdb_test") {
    DestroyDB(dbloc_, DBOptions());
    db_ = NULL;
  }

  Status OpenDb() {
    db_ = new FilesystemDb(options_);
    return db_->Open(dbloc_);
  }

  ~FilesystemDbTest() {  ///
    delete db_;
  }

  std::string dbloc_;
  FilesystemDbOptions options_;
  FilesystemDb* db_;
};

TEST(FilesystemDbTest, OpenAndClose) {  ///
  ASSERT_OK(OpenDb());
}

namespace {  // Db benchmark

// Number of key/values to place in the database.
int FLAGS_num = 1000000;

// Number of concurrent threads to run.
int FLAGS_threads = 1;

// Print histogram of op timings.
bool FLAGS_histogram = false;

// Number of bytes to use as a cache of uncompressed data. Set by main.
int FLAGS_cache_size = -1;

// Maximum number of files to keep open at the same time. Set ny main.
int FLAGS_open_files = -1;

// Bloom filter bits per key. Negative means use default settings. Set by main.
int FLAGS_bloom_bits = -1;

// Sequential or random.
bool FLAGS_seq = false;

// If true, do not destroy the existing database.
bool FLAGS_use_existing_db = false;

// Use the db with the following name.
const char* FLAGS_db = NULL;

void AppendWithSpace(std::string* str, Slice msg) {
  if (msg.empty()) return;
  if (!str->empty()) {
    str->push_back(' ');
  }
  str->append(msg.data(), msg.size());
}

// Performance stats.
class Stats {
 private:
  double start_;
  double finish_;
  double seconds_;
  int done_;
  int next_report_;
  int64_t bytes_;
  double last_op_finish_;
  Histogram hist_;
  std::string message_;

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
  }

  void Merge(const Stats& other) {
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
    finish_ = CurrentMicros();
    seconds_ = (finish_ - start_) * 1e-6;
  }

  void AddMessage(Slice msg) { AppendWithSpace(&message_, msg); }

  void FinishedSingleOp() {
    if (FLAGS_histogram) {
      double now = CurrentMicros();
      double micros = now - last_op_finish_;
      hist_.Add(micros);
      if (micros > 20000) {
        fprintf(stderr, "long op: %.1f micros%30s\r", micros, "");
        fflush(stderr);
      }
      last_op_finish_ = now;
    }

    done_++;
    if (done_ >= next_report_) {
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
      fprintf(stderr, "... finished %d ops%30s\r", done_, "");
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
      snprintf(rate, sizeof(rate), "%6.1f MB/s",
               (bytes_ / 1048576.0) / elapsed);
      extra = rate;
    }
    AppendWithSpace(&extra, message_);

    fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n", name.ToString().c_str(),
            seconds_ * 1e6 / done_, (extra.empty() ? "" : " "), extra.c_str());
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
  int tid;      // 0..n-1 when running in n threads
  Random rand;  // Has different seeds for different threads
  SharedState* shared;
  Stats stats;

  explicit ThreadState(int tid) : tid(tid), rand(1000 + tid), shared(NULL) {}
};
}  // namespace

class Benchmark {
 private:
  FilesystemDbOptions options_;
  FilesystemDb* db_;

  void PrintHeader() {
    PrintEnvironment();
    fprintf(stdout, "Keys:       %d bytes prefix + filename\n",
            Key(0, 0, static_cast<KeyType>(0)).Encode().size());
    fprintf(stdout, "Entries:    %d\n", FLAGS_num);
    PrintWarnings();
    fprintf(stdout, "------------------------------------------------\n");
  }

  void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
    fprintf(
        stdout,
        "WARNING: Optimization is disabled: benchmarks unnecessarily slow\n");
#endif
#ifndef NDEBUG
    fprintf(stdout,
            "WARNING: Assertions are enabled; benchmarks unnecessarily slow\n");
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

  void PrintEnvironment() {
#if defined(__linux)
    time_t now = time(nullptr);
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
    void (Benchmark::*method)(ThreadState*);
    SharedState* shared;
    ThreadState* thread;
  };

  static void ThreadBody(void* v) {
    ThreadArg* arg = reinterpret_cast<ThreadArg*>(v);
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
    (arg->bm->*(arg->method))(thread);
    thread->stats.Stop();

    {
      MutexLock l(&shared->mu);
      shared->num_done++;
      if (shared->num_done >= shared->total) {
        shared->cv.SignalAll();
      }
    }
  }

  void RunBenchmark(int n, Slice name,
                    void (Benchmark::*method)(ThreadState*)) {
    SharedState shared(n);

    ThreadArg* arg = new ThreadArg[n];
    for (int i = 0; i < n; i++) {
      arg[i].bm = this;
      arg[i].method = method;
      arg[i].shared = &shared;
      arg[i].thread = new ThreadState(i);
      arg[i].thread->shared = &shared;
      Env::Default()->StartThread(ThreadBody, &arg[i]);
    }

    shared.mu.Lock();
    while (shared.num_initialized < n) {
      shared.cv.Wait();
    }

    shared.start = true;
    shared.cv.SignalAll();
    while (shared.num_done < n) {
      shared.cv.Wait();
    }
    shared.mu.Unlock();

    for (int i = 1; i < n; i++) {
      arg[0].thread->stats.Merge(arg[i].thread->stats);
    }
    arg[0].thread->stats.Report(name);

    for (int i = 0; i < n; i++) {
      delete arg[i].thread;
    }
    delete[] arg;
  }

  void Write(ThreadState* thread) {
    Status s;
    int64_t bytes = 0;
    Stat stat;
    for (int i = 0; i < FLAGS_num; i++) {
      const int k = FLAGS_seq ? i : int(thread->rand.Next() % FLAGS_num);
      char key[100];
      snprintf(key, sizeof(key), "%016d", k);
      stat.SetInodeNo(k);
      s = db_->Set(DirId(0, 0), Slice(key, 16), stat);
      if (!s.ok()) {
        fprintf(stderr, "put error: %s\n", s.ToString().c_str());
        exit(1);
      }
      thread->stats.FinishedSingleOp();
    }
    thread->stats.AddBytes(bytes);
  }

 public:
  Benchmark() : db_(NULL) {
    if (!FLAGS_use_existing_db) {
      DestroyDB(FLAGS_db, DBOptions());
    }
  }

  ~Benchmark() {  ///
    delete db_;
  }

  void Run() {
    PrintHeader();
    options_.filter_bits_per_key = FLAGS_bloom_bits;
    options_.block_cache_size = FLAGS_cache_size;
    db_ = new FilesystemDb(options_);
    Status s = db_->Open(FLAGS_db);
    if (!s.ok()) {
      fprintf(stderr, "open error: %s\n", s.ToString().c_str());
      exit(1);
    }
    RunBenchmark(FLAGS_threads, "Write", &Benchmark::Write);
  }
};
}  // namespace pdlfs

static void BM_Usage() {
  fprintf(stderr, "Use --bench [options] to run db benchmark.\n");
}

static void BM_Main(int* argc, char*** argv) {
  pdlfs::FLAGS_bloom_bits = pdlfs::FilesystemDbOptions().filter_bits_per_key;
  pdlfs::FLAGS_cache_size = pdlfs::FilesystemDbOptions().block_cache_size;
  std::string default_db_path;

  for (int i = 2; i < *argc; i++) {
    int n;
    char junk;
    if (sscanf((*argv)[i], "--histogram=%d%c", &n, &junk) == 1 &&
        (n == 0 || n == 1)) {
      pdlfs::FLAGS_histogram = n;
    } else if (sscanf((*argv)[i], "--use_existing_db=%d%c", &n, &junk) == 1 &&
               (n == 0 || n == 1)) {
      pdlfs::FLAGS_use_existing_db = n;
    } else if (sscanf((*argv)[i], "--num=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_num = n;
    } else if (sscanf((*argv)[i], "--threads=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_threads = n;
    } else if (sscanf((*argv)[i], "--cache_size=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_cache_size = n;
    } else if (sscanf((*argv)[i], "--bloom_bits=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_bloom_bits = n;
    } else if (sscanf((*argv)[i], "--open_files=%d%c", &n, &junk) == 1) {
      pdlfs::FLAGS_open_files = n;
    } else if (strncmp((*argv)[i], "--db=", 5) == 0) {
      pdlfs::FLAGS_db = (*argv)[i] + 5;
    } else {
      BM_Usage();
      exit(1);
    }
  }

  // Choose a location for the test database if none given with --db=<path>
  if (pdlfs::FLAGS_db == NULL) {
    default_db_path = pdlfs::test::TmpDir() + "/fsdb_bench";
    pdlfs::FLAGS_db = default_db_path.c_str();
  }

  pdlfs::Benchmark benchmark;
  benchmark.Run();
  return;
}

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