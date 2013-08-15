// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/client_socket_handle.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/metrics/histogram.h"
#include "base/logging.h"
#include "net/base/net_errors.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_histograms.h"

namespace net {

ClientSocketHandle::ClientSocketHandle()
    : is_initialized_(false),
      pool_(NULL),
      layered_pool_(NULL),
      is_reused_(false),
      callback_(base::Bind(&ClientSocketHandle::OnIOComplete,
                           base::Unretained(this))),
      is_ssl_error_(false) {}

ClientSocketHandle::~ClientSocketHandle() {
  Reset();
}

void ClientSocketHandle::Reset() {
  ResetInternal(true);
  ResetErrorState();
}

void ClientSocketHandle::ResetInternal(bool cancel) {
  if (group_name_.empty())  // Was Init called?
    return;
  if (is_initialized()) {
    // Because of http://crbug.com/37810 we may not have a pool, but have
    // just a raw socket.
    socket_->NetLog().EndEvent(NetLog::TYPE_SOCKET_IN_USE);
    if (pool_)
      // If we've still got a socket, release it back to the ClientSocketPool so
      // it can be deleted or reused.
      pool_->ReleaseSocket(group_name_, PassSocket(), pool_id_);
  } else if (cancel) {
    // If we did not get initialized yet, we've got a socket request pending.
    // Cancel it.
    pool_->CancelRequest(group_name_, this);
  }
  is_initialized_ = false;
  group_name_.clear();
  is_reused_ = false;
  user_callback_.Reset();
  if (layered_pool_) {
    pool_->RemoveLayeredPool(layered_pool_);
    layered_pool_ = NULL;
  }
  pool_ = NULL;
  idle_time_ = base::TimeDelta();
  init_time_ = base::TimeTicks();
  setup_time_ = base::TimeDelta();
  connect_timing_ = LoadTimingInfo::ConnectTiming();
  pool_id_ = -1;
}

void ClientSocketHandle::ResetErrorState() {
  is_ssl_error_ = false;
  ssl_error_response_info_ = HttpResponseInfo();
  pending_http_proxy_connection_.reset();
}

LoadState ClientSocketHandle::GetLoadState() const {
  CHECK(!is_initialized());
  CHECK(!group_name_.empty());
  // Because of http://crbug.com/37810  we may not have a pool, but have
  // just a raw socket.
  if (!pool_)
    return LOAD_STATE_IDLE;
  return pool_->GetLoadState(group_name_, this);
}

bool ClientSocketHandle::IsPoolStalled() const {
  return pool_->IsStalled();
}

void ClientSocketHandle::AddLayeredPool(LayeredPool* layered_pool) {
  CHECK(layered_pool);
  CHECK(!layered_pool_);
  if (pool_) {
    pool_->AddLayeredPool(layered_pool);
    layered_pool_ = layered_pool;
  }
}

void ClientSocketHandle::RemoveLayeredPool(LayeredPool* layered_pool) {
  CHECK(layered_pool);
  CHECK(layered_pool_);
  if (pool_) {
    pool_->RemoveLayeredPool(layered_pool);
    layered_pool_ = NULL;
  }
}

bool ClientSocketHandle::GetLoadTimingInfo(
    bool is_reused,
    LoadTimingInfo* load_timing_info) const {
  // Only return load timing information when there's a socket.
  if (!socket_)
    return false;

  load_timing_info->socket_log_id = socket_->NetLog().source().id;
  load_timing_info->socket_reused = is_reused;

  // No times if the socket is reused.
  if (is_reused)
    return true;

  load_timing_info->connect_timing = connect_timing_;
  return true;
}

void ClientSocketHandle::SetSocket(scoped_ptr<StreamSocket> s) {
  socket_ = s.Pass();
}

void ClientSocketHandle::OnIOComplete(int result) {
  CompletionCallback callback = user_callback_;
  user_callback_.Reset();
  HandleInitCompletion(result);
  callback.Run(result);
}

scoped_ptr<StreamSocket> ClientSocketHandle::PassSocket() {
  return socket_.Pass();
}

void ClientSocketHandle::HandleInitCompletion(int result) {
  CHECK_NE(ERR_IO_PENDING, result);
  ClientSocketPoolHistograms* histograms = pool_->histograms();
  histograms->AddErrorCode(result);
  if (result != OK) {
    if (!socket_.get())
      ResetInternal(false);  // Nothing to cancel since the request failed.
    else
      is_initialized_ = true;
    return;
  }
  is_initialized_ = true;
  CHECK_NE(-1, pool_id_) << "Pool should have set |pool_id_| to a valid value.";
  setup_time_ = base::TimeTicks::Now() - init_time_;

  histograms->AddSocketType(reuse_type());
  switch (reuse_type()) {
    case ClientSocketHandle::UNUSED:
      histograms->AddRequestTime(setup_time());
      break;
    case ClientSocketHandle::UNUSED_IDLE:
      histograms->AddUnusedIdleTime(idle_time());
      break;
    case ClientSocketHandle::REUSED_IDLE:
      histograms->AddReusedIdleTime(idle_time());
      break;
    default:
      NOTREACHED();
      break;
  }

  // Broadcast that the socket has been acquired.
  // TODO(eroman): This logging is not complete, in particular set_socket() and
  // release() socket. It ends up working though, since those methods are being
  // used to layer sockets (and the destination sources are the same).
  DCHECK(socket_.get());
  socket_->NetLog().BeginEvent(
      NetLog::TYPE_SOCKET_IN_USE,
      requesting_source_.ToEventParametersCallback());
}

}  // namespace net
