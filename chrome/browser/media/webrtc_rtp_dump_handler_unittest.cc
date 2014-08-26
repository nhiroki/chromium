// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "chrome/browser/media/webrtc_rtp_dump_handler.h"
#include "chrome/browser/media/webrtc_rtp_dump_writer.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class FakeDumpWriter : public WebRtcRtpDumpWriter {
 public:
  FakeDumpWriter(size_t max_dump_size,
                 const base::Closure& max_size_reached_callback,
                 bool end_dump_success)
      : WebRtcRtpDumpWriter(base::FilePath(),
                            base::FilePath(),
                            max_dump_size,
                            base::Closure()),
        max_dump_size_(max_dump_size),
        current_dump_size_(0),
        max_size_reached_callback_(max_size_reached_callback),
        end_dump_success_(end_dump_success) {}

  virtual void WriteRtpPacket(const uint8* packet_header,
                              size_t header_length,
                              size_t packet_length,
                              bool incoming) OVERRIDE {
    current_dump_size_ += header_length;
    if (current_dump_size_ > max_dump_size_)
      max_size_reached_callback_.Run();
  }

  virtual void EndDump(RtpDumpType type,
                       const EndDumpCallback& finished_callback) OVERRIDE {
    bool incoming_sucess = end_dump_success_;
    bool outgoing_success = end_dump_success_;

    if (type == RTP_DUMP_INCOMING)
      outgoing_success = false;
    else if (type == RTP_DUMP_OUTGOING)
      incoming_sucess = false;

    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(finished_callback, incoming_sucess, outgoing_success));
  }

 private:
  size_t max_dump_size_;
  size_t current_dump_size_;
  base::Closure max_size_reached_callback_;
  bool end_dump_success_;
};

class WebRtcRtpDumpHandlerTest : public testing::Test {
 public:
  WebRtcRtpDumpHandlerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
    ResetDumpHandler(base::FilePath(), true);
  }

  void ResetDumpHandler(const base::FilePath& dir, bool end_dump_success) {
    handler_.reset(new WebRtcRtpDumpHandler(
        dir.empty() ? base::FilePath(FILE_PATH_LITERAL("dummy")) : dir));

    scoped_ptr<WebRtcRtpDumpWriter> writer(new FakeDumpWriter(
        10,
        base::Bind(&WebRtcRtpDumpHandler::OnMaxDumpSizeReached,
                   base::Unretained(handler_.get())),
        end_dump_success));

    handler_->SetDumpWriterForTesting(writer.Pass());
  }

  void DeleteDumpHandler() { handler_.reset(); }

  void WriteFakeDumpFiles(const base::FilePath& dir,
                          base::FilePath* incoming_dump,
                          base::FilePath* outgoing_dump) {
    *incoming_dump = dir.AppendASCII("recv");
    *outgoing_dump = dir.AppendASCII("send");
    const char dummy[] = "dummy";
    EXPECT_GT(base::WriteFile(*incoming_dump, dummy, arraysize(dummy)), 0);
    EXPECT_GT(base::WriteFile(*outgoing_dump, dummy, arraysize(dummy)), 0);
  }

  MOCK_METHOD2(OnStopDumpFinished,
               void(bool success, const std::string& error));

  MOCK_METHOD0(OnStopOngoingDumpsFinished, void(void));

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  scoped_ptr<WebRtcRtpDumpHandler> handler_;
};

TEST_F(WebRtcRtpDumpHandlerTest, StateTransition) {
  std::string error;

  RtpDumpType types[3];
  types[0] = RTP_DUMP_INCOMING;
  types[1] = RTP_DUMP_OUTGOING;
  types[2] = RTP_DUMP_BOTH;

  for (size_t i = 0; i < arraysize(types); ++i) {
    DVLOG(2) << "Verifying state transition: type = " << types[i];

    // Only StartDump is allowed in STATE_NONE.
    EXPECT_CALL(*this, OnStopDumpFinished(false, testing::_));
    handler_->StopDump(types[i],
                       base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                  base::Unretained(this)));

    WebRtcRtpDumpHandler::ReleasedDumps empty_dumps(handler_->ReleaseDumps());
    EXPECT_TRUE(empty_dumps.incoming_dump_path.empty());
    EXPECT_TRUE(empty_dumps.outgoing_dump_path.empty());
    EXPECT_TRUE(handler_->StartDump(types[i], &error));
    base::RunLoop().RunUntilIdle();

    // Only StopDump is allowed in STATE_STARTED.
    EXPECT_FALSE(handler_->StartDump(types[i], &error));
    EXPECT_FALSE(handler_->ReadyToRelease());

    EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));
    handler_->StopDump(types[i],
                       base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                  base::Unretained(this)));
    base::RunLoop().RunUntilIdle();

    // Only ReleaseDump is allowed in STATE_STOPPED.
    EXPECT_FALSE(handler_->StartDump(types[i], &error));

    EXPECT_CALL(*this, OnStopDumpFinished(false, testing::_));
    handler_->StopDump(types[i],
                       base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                  base::Unretained(this)));
    EXPECT_TRUE(handler_->ReadyToRelease());

    WebRtcRtpDumpHandler::ReleasedDumps dumps(handler_->ReleaseDumps());
    if (types[i] == RTP_DUMP_INCOMING || types[i] == RTP_DUMP_BOTH)
      EXPECT_FALSE(dumps.incoming_dump_path.empty());

    if (types[i] == RTP_DUMP_OUTGOING || types[i] == RTP_DUMP_BOTH)
      EXPECT_FALSE(dumps.outgoing_dump_path.empty());

    base::RunLoop().RunUntilIdle();
    ResetDumpHandler(base::FilePath(), true);
  }
}

TEST_F(WebRtcRtpDumpHandlerTest, StoppedWhenMaxSizeReached) {
  std::string error;

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_INCOMING, &error));

  std::vector<uint8> buffer(100, 0);
  handler_->OnRtpPacket(&buffer[0], buffer.size(), buffer.size(), true);
  base::RunLoop().RunUntilIdle();

  // Dumping should have been stopped, so ready to release.
  WebRtcRtpDumpHandler::ReleasedDumps dumps = handler_->ReleaseDumps();
  EXPECT_FALSE(dumps.incoming_dump_path.empty());
}

TEST_F(WebRtcRtpDumpHandlerTest, PacketIgnoredIfDumpingNotStarted) {
  std::vector<uint8> buffer(100, 0);
  handler_->OnRtpPacket(&buffer[0], buffer.size(), buffer.size(), true);
  handler_->OnRtpPacket(&buffer[0], buffer.size(), buffer.size(), false);
  base::RunLoop().RunUntilIdle();
}

TEST_F(WebRtcRtpDumpHandlerTest, PacketIgnoredIfDumpingStopped) {
  std::string error;

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_INCOMING, &error));

  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));
  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  std::vector<uint8> buffer(100, 0);
  handler_->OnRtpPacket(&buffer[0], buffer.size(), buffer.size(), true);
  base::RunLoop().RunUntilIdle();
}

TEST_F(WebRtcRtpDumpHandlerTest, CannotStartMoreThanFiveDumps) {
  std::string error;

  handler_.reset();

  scoped_ptr<WebRtcRtpDumpHandler> handlers[6];

  for (size_t i = 0; i < arraysize(handlers); ++i) {
    handlers[i].reset(new WebRtcRtpDumpHandler(base::FilePath()));

    if (i < arraysize(handlers) - 1) {
      EXPECT_TRUE(handlers[i]->StartDump(RTP_DUMP_INCOMING, &error));
    } else {
      EXPECT_FALSE(handlers[i]->StartDump(RTP_DUMP_INCOMING, &error));
    }
  }
}

TEST_F(WebRtcRtpDumpHandlerTest, StartStopIncomingThenStartStopOutgoing) {
  std::string error;

  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_)).Times(2);

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_INCOMING, &error));
  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_OUTGOING, &error));
  handler_->StopDump(RTP_DUMP_OUTGOING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  base::RunLoop().RunUntilIdle();
}

TEST_F(WebRtcRtpDumpHandlerTest, StartIncomingStartOutgoingThenStopBoth) {
  std::string error;

  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_INCOMING, &error));
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_OUTGOING, &error));

  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  base::RunLoop().RunUntilIdle();
}

TEST_F(WebRtcRtpDumpHandlerTest, StartBothThenStopIncomingStopOutgoing) {
  std::string error;

  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_)).Times(2);

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));
  handler_->StopDump(RTP_DUMP_OUTGOING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  base::RunLoop().RunUntilIdle();
}

TEST_F(WebRtcRtpDumpHandlerTest, DumpsCleanedUpIfNotReleased) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ResetDumpHandler(temp_dir.path(), true);

  base::FilePath incoming_dump, outgoing_dump;
  WriteFakeDumpFiles(temp_dir.path(), &incoming_dump, &outgoing_dump);

  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));
  handler_->StopDump(RTP_DUMP_BOTH,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));
  base::RunLoop().RunUntilIdle();

  handler_.reset();
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(base::PathExists(incoming_dump));
  EXPECT_FALSE(base::PathExists(outgoing_dump));
}

TEST_F(WebRtcRtpDumpHandlerTest, DumpDeletedIfEndDumpFailed) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Make the writer return failure on EndStream.
  ResetDumpHandler(temp_dir.path(), false);

  base::FilePath incoming_dump, outgoing_dump;
  WriteFakeDumpFiles(temp_dir.path(), &incoming_dump, &outgoing_dump);

  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));
  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_)).Times(2);

  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(base::PathExists(incoming_dump));
  EXPECT_TRUE(base::PathExists(outgoing_dump));

  handler_->StopDump(RTP_DUMP_OUTGOING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(base::PathExists(outgoing_dump));
}

TEST_F(WebRtcRtpDumpHandlerTest, StopOngoingDumpsWhileStoppingDumps) {
  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  testing::InSequence s;
  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));
  EXPECT_CALL(*this, OnStopOngoingDumpsFinished());

  handler_->StopDump(RTP_DUMP_BOTH,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  handler_->StopOngoingDumps(
      base::Bind(&WebRtcRtpDumpHandlerTest::OnStopOngoingDumpsFinished,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  WebRtcRtpDumpHandler::ReleasedDumps dumps(handler_->ReleaseDumps());
  EXPECT_FALSE(dumps.incoming_dump_path.empty());
  EXPECT_FALSE(dumps.outgoing_dump_path.empty());
}

TEST_F(WebRtcRtpDumpHandlerTest, StopOngoingDumpsWhileDumping) {
  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  EXPECT_CALL(*this, OnStopOngoingDumpsFinished());

  handler_->StopOngoingDumps(
      base::Bind(&WebRtcRtpDumpHandlerTest::OnStopOngoingDumpsFinished,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  WebRtcRtpDumpHandler::ReleasedDumps dumps(handler_->ReleaseDumps());
  EXPECT_FALSE(dumps.incoming_dump_path.empty());
  EXPECT_FALSE(dumps.outgoing_dump_path.empty());
}

TEST_F(WebRtcRtpDumpHandlerTest, StopOngoingDumpsWhenAlreadyStopped) {
  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  {
    EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));

    handler_->StopDump(RTP_DUMP_BOTH,
                       base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                  base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  EXPECT_CALL(*this, OnStopOngoingDumpsFinished());
  handler_->StopOngoingDumps(
      base::Bind(&WebRtcRtpDumpHandlerTest::OnStopOngoingDumpsFinished,
                 base::Unretained(this)));
}

TEST_F(WebRtcRtpDumpHandlerTest, StopOngoingDumpsWhileStoppingOneDump) {
  std::string error;
  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  testing::InSequence s;
  EXPECT_CALL(*this, OnStopDumpFinished(true, testing::_));
  EXPECT_CALL(*this, OnStopOngoingDumpsFinished());

  handler_->StopDump(RTP_DUMP_INCOMING,
                     base::Bind(&WebRtcRtpDumpHandlerTest::OnStopDumpFinished,
                                base::Unretained(this)));

  handler_->StopOngoingDumps(
      base::Bind(&WebRtcRtpDumpHandlerTest::OnStopOngoingDumpsFinished,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  WebRtcRtpDumpHandler::ReleasedDumps dumps(handler_->ReleaseDumps());
  EXPECT_FALSE(dumps.incoming_dump_path.empty());
  EXPECT_FALSE(dumps.outgoing_dump_path.empty());
}

TEST_F(WebRtcRtpDumpHandlerTest, DeleteHandlerBeforeStopCallback) {
  std::string error;

  EXPECT_CALL(*this, OnStopOngoingDumpsFinished())
      .WillOnce(testing::InvokeWithoutArgs(
          this, &WebRtcRtpDumpHandlerTest::DeleteDumpHandler));

  EXPECT_TRUE(handler_->StartDump(RTP_DUMP_BOTH, &error));

  handler_->StopOngoingDumps(
      base::Bind(&WebRtcRtpDumpHandlerTest::OnStopOngoingDumpsFinished,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();
}
