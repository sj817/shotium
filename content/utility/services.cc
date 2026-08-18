// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/utility/services.h"

#include <utility>

#include "base/no_destructor.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/public/utility/content_utility_client.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/interface_endpoint_client.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "mojo/public/cpp/bindings/service_factory.h"
#include "services/network/network_service.h"

#if BUILDFLAG(ENABLE_VR) && !BUILDFLAG(IS_ANDROID)
#include "content/services/isolated_xr_device/xr_device_service.h"  // nogncheck
#include "device/vr/public/mojom/isolated_xr_service.mojom.h"       // nogncheck
#endif

#if BUILDFLAG(IS_CHROMEOS) && 0
#include "chromeos/ash/experiences/arc/video_accelerator/oop_arc_video_accelerator_factory.h"
#endif  // BUILDFLAG(IS_CHROMEOS) && 0

#if 0
#include "content/common/features.h"
#include "media/mojo/services/oop_video_decoder_factory_process_service.h"  // nogncheck
#endif

namespace content {
NetworkBinderCreationCallback& GetNetworkBinderCreationCallbackForTesting() {
  static base::NoDestructor<NetworkBinderCreationCallback> callback;
  return *callback;
}

namespace {

auto RunNetworkService(
    mojo::PendingReceiver<network::mojom::NetworkService> receiver) {
  auto binders = std::make_unique<service_manager::BinderRegistry>();
  if (GetNetworkBinderCreationCallbackForTesting()) {
    std::move(GetNetworkBinderCreationCallbackForTesting()).Run(binders.get());
  }
  mojo::InterfaceEndpointClient::SetThreadNameSuffixForMetrics(
      "NetworkServiceOutOfProcess");
  return std::make_unique<network::NetworkService>(
      std::move(binders), std::move(receiver),
      /*delay_initialization_until_set_client=*/true);
}

auto RunDataDecoder(
    mojo::PendingReceiver<data_decoder::mojom::DataDecoderService> receiver) {
  UtilityThread::Get()->EnsureBlinkInitialized();
  return std::make_unique<data_decoder::DataDecoderService>(
      std::move(receiver));
}

auto RunStorageService(
    mojo::PendingReceiver<storage::mojom::StorageService> receiver) {
  return std::make_unique<storage::StorageServiceImpl>(
      std::move(receiver), ChildProcess::current()->io_task_runner());
}

auto RunTracing(
    mojo::PendingReceiver<tracing::mojom::TracingService> receiver) {
  return std::make_unique<tracing::TracingService>(std::move(receiver));
}

#if BUILDFLAG(ENABLE_VR) && !BUILDFLAG(IS_ANDROID)
auto RunXrDeviceService(
    mojo::PendingReceiver<device::mojom::XRDeviceService> receiver) {
  return std::make_unique<device::XrDeviceService>(
      std::move(receiver), ChildProcess::current()->io_task_runner());
}
#endif

#if BUILDFLAG(IS_CHROMEOS) && 0
auto RunOOPArcVideoAcceleratorFactoryService(
    mojo::PendingReceiver<arc::mojom::VideoAcceleratorFactory> receiver) {
  return std::make_unique<arc::OOPArcVideoAcceleratorFactory>(
      std::move(receiver));
}
#endif  // BUILDFLAG(IS_CHROMEOS) && \
        // 0

#if 0
auto RunOOPVideoDecoderFactoryProcessService(
    mojo::PendingReceiver<media::mojom::VideoDecoderFactoryProcess> receiver) {
  return std::make_unique<media::OOPVideoDecoderFactoryProcessService>(
      std::move(receiver), ChildProcess::current()->io_task_runner());
}

auto RunVideoEncodeAcceleratorProviderFactory(
    mojo::PendingReceiver<media::mojom::VideoEncodeAcceleratorProviderFactory>
        receiver) {
  auto factory =
      std::make_unique<media::MojoVideoEncodeAcceleratorProviderFactory>();
  factory->BindReceiver(std::move(receiver));
  return factory;
}

#endif  // (0

}  // namespace

void SetNetworkBinderCreationCallbackForTesting(  // IN-TEST
    NetworkBinderCreationCallback callback) {
  GetNetworkBinderCreationCallbackForTesting() = std::move(callback);
}

void RegisterIOThreadServices(mojo::ServiceFactory& services) {
  // The network service runs on the IO thread because it needs a message
  // loop of type IO that can get notified when pipes have data.
  services.Add(RunNetworkService);

  // Add new IO-thread services above this line.
  GetContentClient()->utility()->RegisterIOThreadServices(services);
}

void RegisterMainThreadServices(mojo::ServiceFactory& services) {
  services.Add(RunDataDecoder);
  services.Add(RunStorageService);
  services.Add(RunTracing);

#if 0
  services.Add(RunOOPVideoDecoderFactoryProcessService);
#endif

#if BUILDFLAG(ENABLE_VR) && !BUILDFLAG(IS_ANDROID)
  services.Add(RunXrDeviceService);
#endif

#if BUILDFLAG(IS_CHROMEOS) && 0
  services.Add(RunOOPArcVideoAcceleratorFactoryService);
#endif  // BUILDFLAG(IS_CHROMEOS) && \
        // 0

#if 0
  services.Add(RunVideoEncodeAcceleratorProviderFactory);
#endif  // 0

  // Add new main-thread services above this line.
  GetContentClient()->utility()->RegisterMainThreadServices(services);
}

}  // namespace content
