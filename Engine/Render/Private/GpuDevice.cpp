#include "Nexum/Render/GpuDevice.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <SDL3/SDL.h>

#include "GpuDeviceAccess.hpp"

namespace nexum::render {

namespace {

using DevicePointer = std::unique_ptr<SDL_GPUDevice, DestroyGpuDevice>;

// The settings SDL reads when it creates a device, released when the creation ends.
class CreationProperties {
public:
    CreationProperties() noexcept : m_id(SDL_CreateProperties()) {}
    CreationProperties(const CreationProperties&) = delete;
    CreationProperties& operator=(const CreationProperties&) = delete;
    ~CreationProperties() {
        if (m_id != 0) {
            SDL_DestroyProperties(m_id);
        }
    }

    [[nodiscard]] SDL_PropertiesID Id() const noexcept { return m_id; }

private:
    SDL_PropertiesID m_id;
};

[[nodiscard]] std::string SdlError() {
    return SDL_GetError();
}

// SDL GPU lists only the backends compiled into the library for this system.
[[nodiscard]] bool IsAvailable(GpuBackend backend) {
    const std::string_view wanted = ToString(backend);
    const int count = SDL_GetNumGPUDrivers();
    for (int index = 0; index < count; ++index) {
        const char* const name = SDL_GetGPUDriver(index);
        if (name != nullptr && wanted == name) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] core::Result<DevicePointer> CreateWithBackend(GpuBackend backend) {
    const std::string name(ToString(backend));
    if (!IsAvailable(backend)) {
        return core::MakeError(name + ": this GPU driver does not exist on this system");
    }
    const CreationProperties properties;
    if (properties.Id() == 0) {
        return core::MakeError(name + ": " + SdlError());
    }
    // SDL starts a backend only if the program can give it shaders. These are the formats that
    // SDL_shadercross produces (docs/adr/0029-sdl3-e-sdl-gpu.md).
    SDL_SetBooleanProperty(properties.Id(), SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(properties.Id(), SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(properties.Id(), SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    // The validation layers belong to the drivers' developer kits, which players and the CI machines do not
    // have.
    SDL_SetBooleanProperty(properties.Id(), SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
    SDL_SetStringProperty(properties.Id(), SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, name.c_str());

    DevicePointer device(SDL_CreateGPUDeviceWithProperties(properties.Id()));
    if (!device) {
        return core::MakeError(name + ": " + SdlError());
    }
    return device;
}

[[nodiscard]] std::string ReadString(SDL_PropertiesID properties, const char* name) {
    return SDL_GetStringProperty(properties, name, "unknown");
}

[[nodiscard]] GpuDeviceDescription Describe(SDL_GPUDevice* device, GpuBackend backend) {
    const SDL_PropertiesID properties = SDL_GetGPUDeviceProperties(device);
    return GpuDeviceDescription{
        .backend = backend,
        .adapterName = ReadString(properties, SDL_PROP_GPU_DEVICE_NAME_STRING),
        .driverName = ReadString(properties, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING),
        .driverVersion = ReadString(properties, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING),
    };
}

[[nodiscard]] std::span<const GpuBackend> Candidates(const GpuDeviceOptions& options) {
    if (options.forcedBackend) {
        return {&*options.forcedBackend, 1};
    }
    return PreferredBackends(platform::CurrentOperatingSystem());
}

} // namespace

// The video system is not used directly: asking for it proves that it is running.
core::Result<GpuDevice> GpuDevice::Create(const platform::VideoSystem& /*video*/,
                                          const GpuDeviceOptions& options) {
    std::string failures;
    for (const GpuBackend backend : Candidates(options)) {
        auto device = CreateWithBackend(backend);
        if (device) {
            auto state = std::make_unique<State>();
            state->description = Describe(device->get(), backend);
            state->device = std::move(*device);
            return GpuDevice(std::move(state));
        }
        failures += (failures.empty() ? "" : "; ") + device.error().message;
    }
    return core::MakeError("no GPU driver could start (" + failures + ")");
}

GpuDevice::GpuDevice(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

GpuDevice::GpuDevice(GpuDevice&& other) noexcept = default;

GpuDevice& GpuDevice::operator=(GpuDevice&& other) noexcept = default;

GpuDevice::~GpuDevice() = default;

const GpuDeviceDescription& GpuDevice::Description() const noexcept {
    return m_state->description;
}

} // namespace nexum::render
