// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_SERIAL_BLUETOOTH_SERIAL_PORT_IMPL_H_
#define SERVICES_DEVICE_SERIAL_BLUETOOTH_SERIAL_PORT_IMPL_H_

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_socket.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "services/device/serial/serial_io_handler.h"
#include "services/device/serial/serial_port_impl.h"

namespace device {

// This class is intended to allow serial communication using a Bluetooth
// SPP device. The Bluetooth device is used to create a Bluetooth socket
// which is closed upon error in any of the interface functions.
class BluetoothSerialPortImpl : public mojom::SerialPort {
 public:
  // Creates of instance of BluetoothSerialPortImpl using a Bluetooth
  // adapter, a Bluetooth device address and a receiver/watcher to
  // create a pipe. The receiver and watcher will own this object.
  static void Create(
      scoped_refptr<BluetoothAdapter> adapter,
      const std::string& address,
      mojo::PendingReceiver<mojom::SerialPort> receiver,
      mojo::PendingRemote<mojom::SerialPortConnectionWatcher> watcher);

  BluetoothSerialPortImpl(
      scoped_refptr<BluetoothAdapter> adapter,
      const std::string& address,
      mojo::PendingReceiver<mojom::SerialPort> receiver,
      mojo::PendingRemote<mojom::SerialPortConnectionWatcher> watcher);
  BluetoothSerialPortImpl(const BluetoothSerialPortImpl&) = delete;
  BluetoothSerialPortImpl& operator=(const BluetoothSerialPortImpl&) = delete;
  ~BluetoothSerialPortImpl() override;

 private:
  // mojom::SerialPort methods:
  void Open(mojom::SerialConnectionOptionsPtr options,
            mojo::PendingRemote<mojom::SerialPortClient> client,
            OpenCallback callback) override;
  void StartWriting(mojo::ScopedDataPipeConsumerHandle consumer) override;
  void StartReading(mojo::ScopedDataPipeProducerHandle producer) override;
  void Flush(mojom::SerialPortFlushMode mode, FlushCallback callback) override;
  void Drain(DrainCallback callback) override;
  void GetControlSignals(GetControlSignalsCallback callback) override;
  void SetControlSignals(mojom::SerialHostControlSignalsPtr signals,
                         SetControlSignalsCallback callback) override;
  void ConfigurePort(mojom::SerialConnectionOptionsPtr options,
                     ConfigurePortCallback callback) override;
  void GetPortInfo(GetPortInfoCallback callback) override;
  void Close(CloseCallback callback) override;

  void WriteToSocket(MojoResult result, const mojo::HandleSignalsState& state);
  void ReadFromSocketAndWriteOut(MojoResult result,
                                 const mojo::HandleSignalsState& state);

  void ReadMore();
  void WriteMore();

  void OnSocketConnected(OpenCallback callback,
                         scoped_refptr<BluetoothSocket> socket);
  void OnSocketConnectedError(OpenCallback callback,
                              const std::string& message);

  void OnBluetoothSocketReceive(base::span<char> pending_write_buffer,
                                int num_bytes_received,
                                scoped_refptr<net::IOBuffer> io_buffer);
  void OnBluetoothSocketReceiveError(
      device::BluetoothSocket::ErrorReason error_reason,
      const std::string& error_message);
  void OnBluetoothSocketSend(int num_bytes_sent);
  void OnBluetoothSocketSendError(const std::string& error_message);

  mojo::Receiver<mojom::SerialPort> receiver_;
  mojo::Remote<mojom::SerialPortConnectionWatcher> watcher_;
  mojo::Remote<mojom::SerialPortClient> client_;

  // Data pipes for input and output.
  mojo::ScopedDataPipeConsumerHandle in_stream_;
  mojo::SimpleWatcher in_stream_watcher_;
  mojo::ScopedDataPipeProducerHandle out_stream_;
  mojo::SimpleWatcher out_stream_watcher_;

  // Holds the callback for a flush or drain until pending operations have been
  // completed.
  FlushCallback read_flush_callback_;
  FlushCallback write_flush_callback_;
  DrainCallback drain_callback_;

  scoped_refptr<BluetoothSocket> bluetooth_socket_;
  const scoped_refptr<BluetoothAdapter> bluetooth_adapter_;
  const std::string address_;

  bool read_pending_ = false;
  bool write_pending_ = false;

  mojom::SerialConnectionOptionsPtr options_;

  base::WeakPtrFactory<BluetoothSerialPortImpl> weak_ptr_factory_{this};
};

}  // namespace device

#endif  // SERVICES_DEVICE_SERIAL_BLUETOOTH_SERIAL_PORT_IMPL_H_
