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
#include "fs.h"

#include "pdlfs-common/testharness.h"

namespace pdlfs {

class FilesystemTest {
 public:
  FilesystemTest() : fsloc_(test::TmpDir() + "/fs_test") {
    DestroyDB(fsloc_, DBOptions());
    fs_ = NULL;
    me_.gid = me_.uid = 1;
    dirmode_ = 0777;
    due_ = -1;
  }

  ~FilesystemTest() {  ///
    delete fs_;
  }

  Status OpenFilesystem() {
    fs_ = new Filesystem(options_);
    return fs_->OpenFilesystem(fsloc_);
  }

  Status Exist(uint64_t dir_id, const std::string& name) {
    LookupStat p;
    p.SetDnodeNo(0);
    p.SetInodeNo(dir_id);
    p.SetZerothServer(0);
    p.SetDirMode(dirmode_);
    p.SetUserId(0);
    p.SetGroupId(0);
    p.SetLeaseDue(due_);
    p.AssertAllSet();
    Stat tmp;
    return fs_->Lstat(me_, p, name, &tmp);
  }

  Status Creat(uint64_t dir_id, const std::string& name) {
    LookupStat p;
    p.SetDnodeNo(0);
    p.SetInodeNo(dir_id);
    p.SetZerothServer(0);
    p.SetDirMode(dirmode_);
    p.SetUserId(0);
    p.SetGroupId(0);
    p.SetLeaseDue(due_);
    p.AssertAllSet();
    Stat tmp;
    return fs_->Mkfle(me_, p, name, 0660, &tmp);
  }

  uint32_t dirmode_;
  uint64_t due_;
  FilesystemOptions options_;
  Filesystem* fs_;
  std::string fsloc_;
  User me_;
};

TEST(FilesystemTest, OpenAndClose) {
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(fs_->TEST_ProbeDir(DirId(0)));
}

TEST(FilesystemTest, Files) {
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(Creat(0, "a"));
  ASSERT_OK(Creat(0, "b"));
  ASSERT_OK(Creat(0, "c"));
  ASSERT_OK(Exist(0, "a"));
  ASSERT_OK(Exist(0, "b"));
  ASSERT_OK(Exist(0, "c"));
}

TEST(FilesystemTest, DuplicateNames) {
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(Creat(0, "a"));
  ASSERT_CONFLICT(Creat(0, "a"));
  ASSERT_OK(Creat(0, "b"));
}

TEST(FilesystemTest, NoDupChecks) {
  options_.skip_name_collision_checks = true;
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(Creat(0, "a"));
  ASSERT_OK(Creat(0, "a"));
}

TEST(FilesystemTest, LeaseExpired) {
  due_ = 0;
  ASSERT_OK(OpenFilesystem());
  ASSERT_ERR(Creat(0, "a"));
}

TEST(FilesystemTest, NoLeaseDueChecks) {
  options_.skip_lease_due_checks = true;
  due_ = 0;
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(Creat(0, "a"));
}

TEST(FilesystemTest, AccessDenied) {
  dirmode_ = 0770;
  ASSERT_OK(OpenFilesystem());
  ASSERT_ERR(Creat(0, "a"));
}

TEST(FilesystemTest, NoPermissionChecks) {
  options_.skip_perm_checks = true;
  dirmode_ = 0770;
  ASSERT_OK(OpenFilesystem());
  ASSERT_OK(Creat(0, "a"));
}

}  // namespace pdlfs

int main(int argc, char* argv[]) {
  return pdlfs::test::RunAllTests(&argc, &argv);
}
