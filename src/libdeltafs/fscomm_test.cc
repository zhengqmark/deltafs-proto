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

#include "pdlfs-common/testharness.h"

namespace pdlfs {

class LokupTest : public rpc::If, public FilesystemIf {
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
      LookupStat* stat) {
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

  virtual Status Call(Message& in, Message& out) RPCNOEXCEPT {
    return rpc::LokupOperation(this)(in, out);
  }

  LookupStat parent_;
  LookupStat stat_;
  Slice name_;
  User who_;
};

TEST(LokupTest, Call) {
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

}  // namespace pdlfs

int main(int argc, char* argv[]) {
  return pdlfs::test::RunAllTests(&argc, &argv);
}
