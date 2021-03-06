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
#pragma once

#include "fsapi.h"

#include "pdlfs-common/rpc.h"

namespace pdlfs {
namespace rpc {
enum { kLokup = 0, kMkdir, kMkfle, kMkfls, kBukin, kLstat, kNumOps };
}

struct LokupOptions {
  const LookupStat* parent;
  Slice name;
  User me;
};
struct LokupRet {
  LookupStat* stat;
};
namespace rpc {
struct LokupOperation {
  LokupOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Lokup(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct LokupCli {
  LokupCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const LokupOptions&, LokupRet*);
  If* rpc_;
};
}  // namespace rpc

struct MkdirOptions {
  const LookupStat* parent;
  Slice name;
  uint32_t mode;
  User me;
};
struct MkdirRet {
  Stat* stat;
};
namespace rpc {
struct MkdirOperation {
  MkdirOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Mkdir(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct MkdirCli {
  MkdirCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const MkdirOptions&, MkdirRet*);
  If* rpc_;
};
}  // namespace rpc

struct MkfleOptions {
  const LookupStat* parent;
  Slice name;
  uint32_t mode;
  User me;
};
struct MkfleRet {
  Stat* stat;
};
namespace rpc {
struct MkfleOperation {
  MkfleOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Mkfle(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct MkfleCli {
  MkfleCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const MkfleOptions&, MkfleRet*);
  If* rpc_;
};
}  // namespace rpc

struct MkflsOptions {
  const LookupStat* parent;
  Slice namearr;
  uint32_t n;
  uint32_t mode;
  User me;
};
struct MkflsRet {
  uint32_t n;
};
namespace rpc {
struct MkflsOperation {
  MkflsOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Mkfls(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct MkflsCli {
  MkflsCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const MkflsOptions&, MkflsRet*);
  If* rpc_;
};
}  // namespace rpc

struct BukinOptions {
  const LookupStat* parent;
  Slice dir;
  User me;
};
struct BukinRet {
  // Empty
};
namespace rpc {
struct BukinOperation {
  BukinOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Bukin(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct BukinCli {
  BukinCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const BukinOptions&, BukinRet*);
  If* rpc_;
};
}  // namespace rpc

struct LstatOptions {
  const LookupStat* parent;
  Slice name;
  User me;
};
struct LstatRet {
  Stat* stat;
};
namespace rpc {
struct LstatOperation {
  LstatOperation(FilesystemIf* fs) : fs_(fs) {}
  Status operator()(If::Message& in, If::Message& out);
  FilesystemIf* fs_;
};
}  // namespace rpc
Status Lstat(FilesystemIf*, rpc::If::Message& in, rpc::If::Message& out);
namespace rpc {
struct LstatCli {
  LstatCli(If* rpc) : rpc_(rpc) {}
  Status operator()(const LstatOptions&, LstatRet*);
  If* rpc_;
};
}  // namespace rpc

}  // namespace pdlfs
