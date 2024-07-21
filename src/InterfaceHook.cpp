#include <InterfaceHook.hpp>
#include <DeviceController.hpp>

#include <stdint.h>
#include <string>

#include <openvr_driver.h>
#include <rcmp.hpp>

void InterfaceHook::Init(vr::IVRDriverContext* pDriverContext)
{
    void** vtable = *((void***)pDriverContext);
#ifdef _WIN32
    int vtable_offset = 0; // on MSVC vtable starts at 0
#else
    int vtable_offset = 1; // on gcc/Clang vtable starts at 1
#endif
    rcmp::hook_indirect_function<void*(*)(void* self, const char* pchInterfaceVersion, void* peError)>(vtable + 0 + vtable_offset, [this](auto orig, void* self, const char* pchInterfaceVersion, void* peError) -> void* {
        void* interfacePtr = orig(self, pchInterfaceVersion, peError);
        GetGenericInterface(interfacePtr, pchInterfaceVersion);
        return interfacePtr;
    });
}

void InterfaceHook::GetGenericInterface(void* interfacePtr, const char* pchInterfaceVersion)
{
   std::string interfaceName(pchInterfaceVersion);

#ifdef _WIN32
   int vtable_offset = 0; // on MSVC vtable starts at 0
#else
   int vtable_offset = 1; // on gcc/Clang vtable starts at 1
#endif

   if (interfaceName == "IVRServerDriverHost_006")
   {
       void** vtable = *((void***)interfacePtr);

       if (!m_IVRServerDriverHostHooked_006)
       {
           rcmp::hook_indirect_function<void(*)(void* self, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)>(vtable + 1 + vtable_offset, [](auto orig, void* self, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize) -> void
           {
               auto pose = newPose;

               auto props = vr::VRProperties()->TrackedDeviceToPropertyContainer(unWhichDevice);

               auto deviceClass = vr::VRProperties()->GetInt32Property(props, vr::ETrackedDeviceProperty::Prop_DeviceClass_Int32);
               auto deviceRole = static_cast<vr::ETrackedControllerRole>(vr::VRProperties()->GetInt32Property(props, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32));
               auto deviceSerial = vr::VRProperties()->GetStringProperty(props, vr::ETrackedDeviceProperty::Prop_SerialNumber_String);

               if (deviceClass == vr::TrackedDeviceClass_Controller && deviceSerial.find("Leap_Hand") == -1)
               {
                   if (pose.deviceIsConnected && pose.poseIsValid && pose.vecPosition[0] != 0 && pose.vecPosition[1] != 0 && pose.vecPosition[2] != 0)
                   {
                       if (DeviceController::get().GetController(deviceRole).m_objectId == 999)
                       {
                           DeviceController::get().UpdateControllerId(deviceRole, unWhichDevice);
                       }
                       else 
                       {
                           if (DeviceController::get().GetController(deviceRole).m_objectId == unWhichDevice)
                           {
                               DeviceController::get().UpdateControllerPose(deviceRole, pose);
                               pose.poseIsValid = false;
                           }
                       }
                   }
                   else 
                   {
                       if (DeviceController::get().GetController(deviceRole).m_objectId == unWhichDevice) 
                       {
                           DeviceController::get().UpdateControllerId(deviceRole, 999);
                       }
                   }
               }

               orig(self, unWhichDevice, pose, unPoseStructSize);
           });

           m_IVRServerDriverHostHooked_006 = true;
       }
   }

   if (interfaceName == "IVRDriverInput_003")
   {
       void** vtable = *((void***)interfacePtr);

       if (!m_IVRDriverInputHooked_003)
       {
           rcmp::hook_indirect_function<vr::EVRInputError(void* self, vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle)>(vtable + 0 + vtable_offset, [](auto orig, void* self,vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle) -> vr::EVRInputError
           {
               vr::EVRInputError result;
               result = orig(self, ulContainer, pchName, pHandle);

               std::string inputName(pchName);
               if (inputName == "/input/system/click")
               {
                   auto deviceRole = vr::VRProperties()->GetInt32Property(ulContainer, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);

                   if (deviceRole == vr::TrackedControllerRole_LeftHand)
                       DeviceController::get().getComponent(0).m_orig = *pHandle;
                   if (deviceRole == vr::TrackedControllerRole_RightHand)
                       DeviceController::get().getComponent(1).m_orig = *pHandle;
               }

               return result;
           });

           rcmp::hook_indirect_function<vr::EVRInputError(void* self, vr::VRInputComponentHandle_t ulComponent, bool bNewValue, double fTimeOffset)>(vtable + 1 + vtable_offset, [](auto orig, void* self, vr::VRInputComponentHandle_t ulComponent, bool bNewValue, double fTimeOffset) -> vr::EVRInputError
           {
               for (auto component : DeviceController::get().GetComponents())
               {
                   if (component.second.m_orig == ulComponent)
                   {
                       return orig(self, component.second.m_override, bNewValue, fTimeOffset);
                   }
               }

               return orig(self, ulComponent, bNewValue, fTimeOffset);
           });

           m_IVRDriverInputHooked_003 = true;
       }
   }
}